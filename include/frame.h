#ifndef FRAME_H
#define FRAME_H

#include <stddef.h>
#include <stdint.h>

#define FRAME_MAX_PAYLOAD 1024u
#define FRAME_OVERHEAD 8u
#define FRAME_MAX_SIZE (FRAME_MAX_PAYLOAD + FRAME_OVERHEAD)

typedef enum {
    FRAME_OK = 0,
    FRAME_ERR_BAD_SYNC,
    FRAME_ERR_BAD_LENGTH,
    FRAME_ERR_CRC_MISMATCH,
    FRAME_ERR_BUFFER_TOO_SMALL,
    FRAME_ERR_INVALID_ARGUMENT
} frame_status_t;

uint16_t frame_crc16_ccitt_false(const uint8_t *data, size_t length);

frame_status_t frame_encode(const uint8_t *payload,
                            size_t payload_length,
                            uint8_t *frame,
                            size_t frame_capacity,
                            size_t *frame_length);

frame_status_t frame_decode(const uint8_t *frame,
                            size_t frame_length,
                            uint8_t *payload,
                            size_t payload_capacity,
                            size_t *payload_length);

#endif
