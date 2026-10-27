#include "frame.h"
#include "version.h"

#include <stdio.h>
#include <string.h>

static int hex_digit(char digit)
{
    if (digit >= '0' && digit <= '9') {
        return digit - '0';
    }
    if (digit >= 'a' && digit <= 'f') {
        return digit - 'a' + 10;
    }
    if (digit >= 'A' && digit <= 'F') {
        return digit - 'A' + 10;
    }
    return -1;
}

static int parse_hex(const char *text, uint8_t *bytes, size_t capacity,
                     size_t *length)
{
    size_t text_length = strlen(text);

    if (text_length % 2 != 0 || text_length / 2 > capacity) {
        return 0;
    }
    for (size_t i = 0; i < text_length / 2; ++i) {
        int high = hex_digit(text[2 * i]);
        int low = hex_digit(text[2 * i + 1]);

        if (high < 0 || low < 0) {
            return 0;
        }
        bytes[i] = (uint8_t)((high << 4) | low);
    }
    *length = text_length / 2;
    return 1;
}

static const char *error_message(frame_status_t status)
{
    switch (status) {
    case FRAME_OK:
        return "success";
    case FRAME_ERR_BAD_SYNC:
        return "bad sync marker";
    case FRAME_ERR_BAD_LENGTH:
        return "bad frame or payload length";
    case FRAME_ERR_CRC_MISMATCH:
        return "CRC mismatch";
    case FRAME_ERR_BUFFER_TOO_SMALL:
        return "output buffer too small";
    case FRAME_ERR_INVALID_ARGUMENT:
        return "invalid argument";
    }
    return "unknown error";
}

int main(int argc, char **argv)
{
    uint8_t input[FRAME_MAX_SIZE];
    uint8_t output[FRAME_MAX_SIZE];
    size_t input_length = 0;
    size_t output_length = 0;
    frame_status_t status;

    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        return puts("tlmframe " TLMFRAME_VERSION " (" TLMFRAME_COMMIT_HASH ")") == EOF ? 1 : 0;
    }
    if (argc != 3 || (strcmp(argv[1], "encode") != 0 &&
                      strcmp(argv[1], "decode") != 0)) {
        fprintf(stderr, "Usage: tlmframe encode <hex> | decode <hex> | --version\n");
        return 1;
    }
    if (!parse_hex(argv[2], input, sizeof(input), &input_length)) {
        fprintf(stderr, "Invalid hex: use an even number of hex digits, at most %u bytes\n",
                FRAME_MAX_SIZE);
        return 1;
    }

    if (strcmp(argv[1], "encode") == 0) {
        status = frame_encode(input, input_length, output, sizeof(output),
                              &output_length);
    } else {
        status = frame_decode(input, input_length, output, sizeof(output),
                              &output_length);
    }
    if (status != FRAME_OK) {
        fprintf(stderr, "%s\n", error_message(status));
        return 1;
    }

    for (size_t i = 0; i < output_length; ++i) {
        printf("%02X", (unsigned int)output[i]);
    }
    putchar('\n');
    return ferror(stdout) ? 1 : 0;
}
