#include "frame.h"

#include <string.h>

static const uint8_t sync_marker[] = {0x1A, 0xCF, 0xFC, 0x1D};

uint16_t frame_crc16_ccitt_false(const uint8_t *data, size_t length)
{
    uint16_t crc = 0xFFFFu;

    for (size_t i = 0; i < length; ++i) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8);

        for (unsigned int bit = 0; bit < 8u; ++bit) {
            if ((crc & 0x8000u) != 0u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }

    return crc;
}

frame_status_t frame_encode(const uint8_t *payload,
                            size_t payload_length,
                            uint8_t *frame,
                            size_t frame_capacity,
                            size_t *frame_length)
{
    if (frame_length == NULL) {
        return FRAME_ERR_INVALID_ARGUMENT;
    }
    *frame_length = 0;

    if (payload == NULL || frame == NULL) {
        return FRAME_ERR_INVALID_ARGUMENT;
    }
    if (payload_length > FRAME_MAX_PAYLOAD) {
        return FRAME_ERR_BAD_LENGTH;
    }

    size_t required_size = payload_length + FRAME_OVERHEAD;
    if (frame_capacity < required_size) {
        return FRAME_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(frame, sync_marker, sizeof sync_marker);
    frame[4] = (uint8_t)(payload_length >> 8);
    frame[5] = (uint8_t)payload_length;
    memcpy(frame + 6, payload, payload_length);

    uint16_t crc = frame_crc16_ccitt_false(frame + 4, payload_length + 2u);
    size_t crc_offset = payload_length + 6u;
    frame[crc_offset] = (uint8_t)(crc >> 8);
    frame[crc_offset + 1u] = (uint8_t)crc;

    *frame_length = required_size;
    return FRAME_OK;
}

frame_status_t frame_decode(const uint8_t *frame,
                            size_t frame_length,
                            uint8_t *payload,
                            size_t payload_capacity,
                            size_t *payload_length)
{
    if (payload_length == NULL) {
        return FRAME_ERR_INVALID_ARGUMENT;
    }
    *payload_length = 0;

    if (frame == NULL || payload == NULL) {
        return FRAME_ERR_INVALID_ARGUMENT;
    }
    if (frame_length < FRAME_OVERHEAD) {
        return FRAME_ERR_BAD_LENGTH;
    }
    if (memcmp(frame, sync_marker, sizeof sync_marker) != 0) {
        return FRAME_ERR_BAD_SYNC;
    }

    size_t declared_length = ((size_t)frame[4] << 8) | frame[5];
    if (declared_length > FRAME_MAX_PAYLOAD ||
        frame_length != declared_length + FRAME_OVERHEAD) {
        return FRAME_ERR_BAD_LENGTH;
    }

    size_t crc_offset = declared_length + 6u;
    uint16_t received_crc = (uint16_t)(((uint16_t)frame[crc_offset] << 8) |
                                       frame[crc_offset + 1u]);
    uint16_t computed_crc = frame_crc16_ccitt_false(frame + 4,
                                                   declared_length + 2u);
    if (received_crc != computed_crc) {
        return FRAME_ERR_CRC_MISMATCH;
    }
    if (payload_capacity < declared_length) {
        return FRAME_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(payload, frame + 6, declared_length);
    *payload_length = declared_length;
    return FRAME_OK;
}
