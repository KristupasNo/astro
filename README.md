# astro

small c lib + cli (tlmframe) that packs bytes into a frame (sync marker, length, payload, crc16) and decodes it back.
made it to learn ci/cd for c - cmake, cross compile to arm64, tests in qemu, version from git, reproducible builds

build (needs docker):

    docker build --platform linux/amd64 --target artifacts --output type=local,dest=build/release .

builds x86_64 + aarch64, debug + release, runs tests (arm ones thru qemu-user).
everything gets built twice in diff folders and sha256 has to match

output goes to build/release - 2 binaries, SHA256SUMS, BUILDINFO.txt

ci.yml - same build on push to main / PRs
release.yml - on v* tag builds and uploads files to github release

    tlmframe encode abcd                  -> 1ACFFC1D0002ABCD23C5
    tlmframe decode 1ACFFC1D0002ABCD23C5  -> ABCD
    tlmframe --version
