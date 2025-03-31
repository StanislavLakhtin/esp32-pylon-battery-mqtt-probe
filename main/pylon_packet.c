#include "pylon_packet.h"
#include <string.h>
#include <ctype.h>

static inline uint8_t hex_char_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0xFF;
}

static uint8_t compute_len_chksum(uint8_t len_id_hi, uint8_t len_id_mid, uint8_t len_id_lo) {
    uint8_t digits[6] = {
        (len_id_hi >> 4) & 0x0F, len_id_hi & 0x0F,
        (len_id_mid >> 4) & 0x0F, len_id_mid & 0x0F,
        (len_id_lo >> 4) & 0x0F, len_id_lo & 0x0F
    };

    uint8_t sum = 0;
    for (int i = 0; i < 6; i++) sum += digits[i];

    uint8_t mod = sum % 16;
    uint8_t chksum = (uint8_t)(~mod + 1);
    return chksum;
}

bool pylon_decode_ascii_hex(const char *input, size_t len_ascii, PylonPacketRaw *out) {
    if (!input || !out ) return false;

    size_t len_bin = len_ascii / 2;

    uint8_t buffer[PYLON_MAX_DATA_BYTES] = {0};
    for (size_t i = 0; i < len_bin; i++) {
        uint8_t hi = hex_char_to_nibble(input[i * 2]);
        uint8_t lo = hex_char_to_nibble(input[i * 2 + 1]);
        if (hi == 0xFF || lo == 0xFF) return false; // недопустимый символ
        buffer[i] = (hi << 4) | lo;
    }

    size_t index = 0;
    out->version = buffer[index++];
    out->address = buffer[index++];
    uint8_t cid1 = buffer[index++];
    uint8_t cid2 = buffer[index++];
    out->cid = ((cid1 & 0xF0) >> 4) * 16 + (cid2 & 0x0F); // уплотнение CID

    uint8_t len_chksum = buffer[index++];
    uint8_t len_id_hi = buffer[index++];
    uint8_t len_id_mid = buffer[index++];
    uint8_t len_id_lo = buffer[index++];

    uint32_t len_id = ((uint32_t)len_id_hi << 16) | ((uint32_t)len_id_mid << 8) | len_id_lo;

    if (compute_len_chksum(len_id_hi, len_id_mid, len_id_lo) != len_chksum) {
        out->valid = false;
        return false;
    }

    out->length = len_id;
    index++;

    if (out->length > PYLON_MAX_DATA_BYTES - index - 3) return false; // закодированная длинна больше, чем длинна пакета
    if ( index + out->length + 3 > len_bin ) return false; // длинна больше размера структуры

    memcpy(out->data, &buffer[index], out->length);
    index += out->length;

    out->checksum = buffer[index++];

    // проверка контрольной суммы (XOR от version до последнего байта data)
    uint8_t calc_checksum = 0;
    for (size_t i = 0; i < 2 + out->length - 1; i++) { // length байт не включается в checksum
        calc_checksum ^= buffer[1 + i];
    }

    out->valid = (calc_checksum == out->checksum);
    return out->valid;
}

