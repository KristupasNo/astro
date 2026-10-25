#include "frame.h"

#include <stdio.h>
#include <string.h>

static int failures;

/* Fixed bytes catch matching mistakes in the encoder and decoder. */
static const uint8_t reference_frame[] = {
    0x1A, 0xCF, 0xFC, 0x1D, 0x00, 0x02, 0xAB, 0xCD, 0x23, 0xC5
};

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        failures++;
    }
}

static void test_crc(void)
{
    const uint8_t input[] = "123456789";

    check(frame_crc16_ccitt_false(input, sizeof(input) - 1u) == 0x29B1u,
          "CRC known-answer vector");
    check(frame_crc16_ccitt_false(NULL, 0) == 0xFFFFu, "CRC of empty input");
}

static void test_wire_format(void)
{
    const uint8_t payload[] = {0xAB, 0xCD};
    uint8_t frame[sizeof(reference_frame)] = {0};
    uint8_t decoded[sizeof(payload)] = {0};
    size_t frame_length = 0;
    size_t payload_length = 0;

    check(frame_encode(payload, sizeof(payload), frame, sizeof(frame),
                       &frame_length) == FRAME_OK, "encode fixed payload");
    check(frame_length == sizeof(reference_frame) &&
          memcmp(frame, reference_frame, sizeof(reference_frame)) == 0,
          "encoded frame matches fixed wire bytes");
    check(frame_decode(reference_frame, sizeof(reference_frame), decoded,
                       sizeof(decoded), &payload_length) == FRAME_OK,
          "decode fixed wire bytes");
    check(payload_length == sizeof(payload) &&
          memcmp(decoded, payload, sizeof(payload)) == 0,
          "decoded fixed payload matches");
}

static void test_round_trips(void)
{
    uint8_t payload[FRAME_MAX_PAYLOAD];
    uint8_t frame[FRAME_MAX_SIZE];
    uint8_t decoded[FRAME_MAX_PAYLOAD];
    const size_t lengths[] = {0, 1, 2, FRAME_MAX_PAYLOAD};

    for (size_t i = 0; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)i;
    }
    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        size_t frame_length = 0;
        size_t payload_length = 0;
        frame_status_t status = frame_encode(payload, lengths[i], frame,
                                             sizeof(frame), &frame_length);
        check(status == FRAME_OK, "round-trip encode");
        if (status != FRAME_OK) {
            continue;
        }
        check(frame_length == lengths[i] + FRAME_OVERHEAD, "round-trip frame length");
        status = frame_decode(frame, frame_length, decoded, lengths[i], &payload_length);
        check(status == FRAME_OK, "round-trip decode");
        if (status == FRAME_OK) {
            check(payload_length == lengths[i] &&
                  memcmp(decoded, payload, lengths[i]) == 0, "round-trip payload");
        }
    }
}

static void test_bad_frames(void)
{
    uint8_t frame[sizeof(reference_frame) + 1u] = {0};
    uint8_t payload[2] = {0xAA, 0xBB};
    size_t length = 99;

    memcpy(frame, reference_frame, sizeof(reference_frame));
    frame[0] ^= 1u;
    check(frame_decode(frame, sizeof(reference_frame), payload, sizeof(payload),
                       &length) == FRAME_ERR_BAD_SYNC, "wrong sync marker");
    check(length == 0, "failed decode clears output length");

    memcpy(frame, reference_frame, sizeof(reference_frame));
    frame[6] ^= 1u;
    check(frame_decode(frame, sizeof(reference_frame), payload, sizeof(payload),
                       &length) == FRAME_ERR_CRC_MISMATCH, "flipped payload bit");
    check(payload[0] == 0xAA && payload[1] == 0xBB,
          "CRC failure leaves output buffer unchanged");

    memcpy(frame, reference_frame, sizeof(reference_frame));
    frame[9] ^= 1u;
    check(frame_decode(frame, sizeof(reference_frame), payload, sizeof(payload),
                       &length) == FRAME_ERR_CRC_MISMATCH, "flipped CRC bit");

    for (size_t i = 0; i < sizeof(reference_frame); i++) {
        check(frame_decode(reference_frame, i, payload, sizeof(payload), &length)
              == FRAME_ERR_BAD_LENGTH, "truncated frame");
    }

    memcpy(frame, reference_frame, sizeof(reference_frame));
    frame[5] = 1;
    check(frame_decode(frame, sizeof(reference_frame), payload, sizeof(payload),
                       &length) == FRAME_ERR_BAD_LENGTH, "wrong declared length");
    frame[4] = 4;
    frame[5] = 1;
    check(frame_decode(frame, sizeof(reference_frame), payload, sizeof(payload),
                       &length) == FRAME_ERR_BAD_LENGTH, "declared payload exceeds maximum");

    memcpy(frame, reference_frame, sizeof(reference_frame));
    check(frame_decode(frame, sizeof(frame), payload, sizeof(payload), &length)
          == FRAME_ERR_BAD_LENGTH, "trailing byte after frame");
}

static void test_buffers(void)
{
    uint8_t payload[FRAME_MAX_PAYLOAD + 1u] = {0};
    uint8_t frame[FRAME_MAX_SIZE] = {0xAA};
    size_t length = 99;

    check(frame_encode(payload, sizeof(payload), frame, sizeof(frame), &length)
          == FRAME_ERR_BAD_LENGTH, "encode payload exceeds maximum");
    check(length == 0, "failed encode clears output length");
    check(frame_encode(payload, 1, frame, FRAME_OVERHEAD, &length)
          == FRAME_ERR_BUFFER_TOO_SMALL, "encode buffer too small");
    check(frame[0] == 0xAA, "failed encode leaves output buffer unchanged");
    payload[0] = 0xBB;
    check(frame_decode(reference_frame, sizeof(reference_frame), payload, 1,
                       &length) == FRAME_ERR_BUFFER_TOO_SMALL, "decode buffer too small");
    check(payload[0] == 0xBB, "small decode buffer remains unchanged");
}

static void test_arguments(void)
{
    uint8_t payload[2] = {0};
    uint8_t frame[FRAME_MAX_SIZE] = {0};
    size_t length = 0;

    check(frame_encode(NULL, 0, frame, sizeof(frame), &length)
          == FRAME_ERR_INVALID_ARGUMENT, "encode null payload");
    check(frame_encode(payload, sizeof(payload), NULL, sizeof(frame), &length)
          == FRAME_ERR_INVALID_ARGUMENT, "encode null output buffer");
    check(frame_encode(payload, sizeof(payload), frame, sizeof(frame), NULL)
          == FRAME_ERR_INVALID_ARGUMENT, "encode null output length");
    check(frame_decode(NULL, sizeof(reference_frame), payload, sizeof(payload), &length)
          == FRAME_ERR_INVALID_ARGUMENT, "decode null frame");
    check(frame_decode(reference_frame, sizeof(reference_frame), NULL, 0, &length)
          == FRAME_ERR_INVALID_ARGUMENT, "decode null output buffer");
    check(frame_decode(reference_frame, sizeof(reference_frame), payload, sizeof(payload), NULL)
          == FRAME_ERR_INVALID_ARGUMENT, "decode null output length");
}

int main(void)
{
    test_crc();
    test_wire_format();
    test_round_trips();
    test_bad_frames();
    test_buffers();
    test_arguments();
    if (failures != 0) {
        fprintf(stderr, "%d checks failed\n", failures);
        return 1;
    }
    puts("All frame tests passed");
    return 0;
}
