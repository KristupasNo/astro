#!/bin/sh
set -eu

test "$(uname -m)" = x86_64
SOURCE_DATE_EPOCH=$(git log -1 --format=%ct)
export SOURCE_DATE_EPOCH

# Different source and build paths expose paths embedded in the binaries.
for copy in first second; do
    mkdir -p "/tmp/$copy/source"
    cp -R /work/. "/tmp/$copy/source/"
done

mkdir -p /artifacts
for arch in x86_64 aarch64; do
    for config in Debug Release; do
        for copy in first second; do
            source="/tmp/$copy/source"
            build="/tmp/$copy/build/$arch/$config"
            if [ "$arch" = aarch64 ]; then
                cmake -S "$source" -B "$build" -DCMAKE_BUILD_TYPE="$config" \
                    -DCMAKE_TOOLCHAIN_FILE="$source/cmake/aarch64-linux-gnu.cmake"
            else
                cmake -S "$source" -B "$build" -DCMAKE_BUILD_TYPE="$config"
            fi
            cmake --build "$build" --parallel
            ctest --test-dir "$build" --output-on-failure
        done

        for file in tlmframe libframe.a; do
            first_hash=$(sha256sum "/tmp/first/build/$arch/$config/$file")
            first_hash=$(printf '%s' "$first_hash" | cut -d ' ' -f 1)
            second_hash=$(sha256sum "/tmp/second/build/$arch/$config/$file")
            second_hash=$(printf '%s' "$second_hash" | cut -d ' ' -f 1)
            if [ "$first_hash" != "$second_hash" ]; then
                echo "Reproducibility check failed: $arch $config $file" >&2
                exit 1
            fi
            printf 'Reproducible: %s %s %s %s\n' "$arch" "$config" "$file" "$first_hash"
        done
    done
    cp "/tmp/first/build/$arch/Release/tlmframe" "/artifacts/tlmframe-linux-$arch"
done

{
    printf 'Version: %s\n' "$(git describe --tags --always --dirty)"
    printf 'Commit: %s\n' "$(git rev-parse HEAD)"
    printf 'SOURCE_DATE_EPOCH: %s\n' "$SOURCE_DATE_EPOCH"
    printf 'Base image: %s\n' "$(awk 'NR == 1 { print $2 }' Dockerfile)"
    printf '\nBuild packages:\n'
    dpkg-query -W gcc gcc-13 gcc-aarch64-linux-gnu gcc-13-aarch64-linux-gnu \
        binutils binutils-aarch64-linux-gnu cmake make git qemu-user \
        libc6-dev libc6-dev-arm64-cross
} > /artifacts/BUILDINFO.txt

cd /artifacts
sha256sum tlmframe-linux-x86_64 tlmframe-linux-aarch64 BUILDINFO.txt > SHA256SUMS
