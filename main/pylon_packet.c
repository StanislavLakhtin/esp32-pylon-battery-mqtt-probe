#include "pylon_packet.h"
#include <string.h>
#include <ctype.h>

static inline uint8_t hex_char_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0xFF;
}

static inline uint8_t hex_to_uint8(char *c) {
  return hex_char_to_nibble(*c)<<4 | hex_char_to_nibble(*(c+1));
}

static inline uint16_t ascii_hex2_to_u16(const uint8_t *p) {
    return (hex_char_to_nibble(p[0]) << 4) | hex_char_to_nibble(p[1]);
}

static inline uint16_t ascii_hex3_to_u16(const uint8_t *p) {
    return (hex_char_to_nibble(p[0]) << 8) | (hex_char_to_nibble(p[1]) << 4) | hex_char_to_nibble(p[2]);
}

static inline uint16_t ascii_hex4_to_u16(const uint8_t *p) {
    return (hex_char_to_nibble(p[0]) << 12) | (hex_char_to_nibble(p[1]) << 8) |
           (hex_char_to_nibble(p[2]) << 4) | hex_char_to_nibble(p[3]);
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

uint16_t compute_checksum_ascii(const char *ascii, size_t len_ascii_without_soi_eoi) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len_ascii_without_soi_eoi; i++) {
        sum += (uint8_t)ascii[i];
    }

    uint16_t mod = (uint16_t)(sum & 0xFFFF);
    uint16_t checksum = (uint16_t)(~mod + 1);
    return checksum;
}

bool pylon_decode_ascii_hex(const char *input, size_t len_ascii, PylonPacketRaw *out) {
    if (!input || !out ) return false;

    if (input[0] != 0x7e ||
        input[len_ascii-1] != 0x0d) return false; // начало и конец пакета не соответствуют спецификации

    size_t len_bin = (len_ascii - 2) / 2; // -SOI -EOI

    uint8_t buffer[PYLON_MAX_DATA_BYTES] = {0};
    for (size_t i = 1; i < len_bin; i++) {
        uint8_t hi = hex_char_to_nibble(input[i * 2]);
        uint8_t lo = hex_char_to_nibble(input[i * 2 + 1]);
        if (hi == 0xFF || lo == 0xFF) return false; // недопустимый символ
        buffer[i-1] = (hi << 4) | lo;
    }

    uint16_t index = 0; // skip SOI

    out->version = buffer[index++];
    out->address = buffer[index++];
    out->cid1 = buffer[index++];
    out->cid2 = buffer[index++];

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

    uint16_t compute_checksum = compute_checksum_ascii(input + 2, len_ascii - 2);
    out->valid = (compute_checksum == out->checksum);

    return out->valid;
}

static inline bool read_field_u16(const uint8_t *info, size_t len, size_t *index, uint16_t *out) {
    if (*index + 4 > len) return false;
    *out = ascii_hex4_to_u16(&info[*index]);
    *index += 4;
    return true;
}

static inline bool read_field_u8(const uint8_t *info, size_t len, size_t *index, uint8_t *out) {
    if (*index + 2 > len) return false;
    *out = (uint8_t)ascii_hex2_to_u16(&info[*index]);
    *index += 2;
    return true;
}

bool pylon_parse_info_payload(const uint8_t *info, size_t len, PylonBatteryStatus *out) {
    if (!info || !out ) return false;

    size_t index = 0;
    if (!read_field_u16(info, len, &index, &out->modules)) return false;
    if (!read_field_u8(info, len, &index, &out->cell_count)) return false;
    if (out->cell_count > PYLON_MAX_CELLS) return false;
    for (int i = 0; i < out->cell_count; i++) {
        if (!read_field_u16(info, len, &index, &out->cell_voltage_mV[i])) return false;
    }
    if (!read_field_u8(info, len, &index, &out->temperature_count)) return false;
    if (out->temperature_count > PYLON_MAX_TEMPS) return false;
    for (int i = 0; i < out->temperature_count; i++) {
        uint16_t temp;
        if (!read_field_u16(info, len, &index, &temp)) return false;
        out->temperatures_c[i] = (int16_t)temp;
    }
    uint16_t tmp16;
    if (!read_field_u16(info, len, &index, &tmp16)) return false;
    out->current_mA = (int16_t)tmp16;

    if (!read_field_u16(info, len, &index, &out->total_voltage_mV)) return false;
    if (!read_field_u16(info, len, &index, &out->remaining_capacity_ah)) return false;

    uint8_t tmp8;
    if (!read_field_u8(info, len, &index, &tmp8)) return false;
    out->user_defined_number = tmp8;

    if (!read_field_u16(info, len, &index, &out->total_capacity_ah)) return false;
    if (!read_field_u16(info, len, &index, &out->cycle_count)) return false;

    if (!read_field_u16(info, len, &index, &out->batteryCapacity)) return false;
    if (!read_field_u16(info, len, &index, &out->currentCapacity)) return false;
    if (!read_field_u16(info, len, &index, &out->userVoltage0)) return false;
    if (!read_field_u16(info, len, &index, &out->userVoltage1)) return false;
    if (!read_field_u16(info, len, &index, &out->userVoltage2)) return false;
    if (!read_field_u16(info, len, &index, &out->userVoltage3)) return false;
    if (!read_field_u16(info, len, &index, &out->userVoltage4)) return false;
    if (!read_field_u16(info, len, &index, &out->unknown)) return false;

    return true;
}

