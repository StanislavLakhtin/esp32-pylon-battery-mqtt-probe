/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#include "pylon_packet.h"
#include "esp_log.h"
#include <string.h>
#include <ctype.h>

static const char *TAG = "pylon_packet";

static inline uint8_t hex_char_to_nibble(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0xFF;
}

static inline uint8_t hex_to_uint8(const char *c) {
    return hex_char_to_nibble(*c) << 4 | hex_char_to_nibble(*(c + 1));
}

static uint8_t copy_ascii_to_hex(const char *in, char *out, size_t ascii_len) {
    size_t hex_len = ascii_len / 2;
    for (size_t i = 0; i < hex_len; i++) {
        out[i] = hex_to_uint8(&in[i * 2]);
    }
    return hex_len;
}

static inline uint16_t ascii_hex2_to_u16(const char *p) {
    return (hex_char_to_nibble(p[0]) << 4) | hex_char_to_nibble(p[1]);
}

static inline uint16_t ascii_hex3_to_u16(const char *p) {
    return (hex_char_to_nibble(p[0]) << 8) | (hex_char_to_nibble(p[1]) << 4) | hex_char_to_nibble(p[2]);
}

static inline uint16_t ascii_hex4_to_u16(const char *p) {
    return (hex_char_to_nibble(p[0]) << 12) | (hex_char_to_nibble(p[1]) << 8) |
           (hex_char_to_nibble(p[2]) << 4) | hex_char_to_nibble(p[3]);
}

uint16_t pylon_parse_length_field(const char *ptr) {
    if (!ptr) {
        ESP_LOGE(TAG, "Invalid pointer");
        return 0xFFFF; // недопустимый указатель
    }

    uint8_t len_chksum = hex_char_to_nibble(ptr[0]);
    uint8_t byte1 = hex_char_to_nibble(ptr[1]);
    uint8_t byte2 = hex_char_to_nibble(ptr[2]);
    uint8_t byte3 = hex_char_to_nibble(ptr[3]);

    ESP_LOGD(TAG,"bytes for length field: %02X (check) - %02X %02X %02X", len_chksum, byte1, byte2, byte3);

    uint16_t len_id = byte1 << 8 | byte2 << 4 | byte3;

    uint8_t computed_chk = (~( (byte1 + byte2 + byte3) % 16) & 0x0F) + 1;

    if (computed_chk != len_chksum) {
        ESP_LOGI(TAG, "Checksum error for length %04X: expected %02X, got %02X", len_id, computed_chk, len_chksum);
        return 0xFFFF; // ошибка контрольной суммы
    }

    return len_id;
}

uint16_t compute_checksum_ascii(const char *ascii, size_t len_ascii_without_soi_eoi) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len_ascii_without_soi_eoi; i++) {
        sum += (uint8_t) ascii[i];
    }

    uint16_t mod = (uint16_t) (sum & 0xFFFF);
    uint16_t checksum = (uint16_t) (~mod + 1);
    return checksum;
}

bool pylon_decode_ascii_hex(const char *input, size_t len_ascii, PylonPacketRaw *out) {
    if (!input || !out) {
        ESP_LOGE(TAG, "Invalid input or output pointer");
        return false; // недопустимый указатель
    }

    if (input[0] != 0x7e || input[len_ascii - 1] != 0x0d) {
        ESP_LOGE(TAG, "Invalid packet start or end");
        return false; // начало и конец пакета не соответствуют спецификации
    }

    uint16_t index = 1; // skip SOI

    out->version = hex_to_uint8(&input[index]);
    index += 2;
    out->address = hex_to_uint8(&input[index]);
    index += 2;
    out->cid1 = hex_to_uint8(&input[index]);
    index += 2;
    out->cid2 = hex_to_uint8(&input[index]);
    index += 2;
    if (out->cid1 != 0x46 || out->cid2 != 0x00) {
        ESP_LOGE(TAG, "Unknown CID: %02X%02X", out->cid1, out->cid2);
        return false; // неизвестный CID
    }
    ESP_LOG_BUFFER_HEXDUMP(TAG, input, len_ascii, ESP_LOG_DEBUG);
    uint16_t length_to_copy = pylon_parse_length_field(&input[index]);
    index += 4; 
    if (length_to_copy == 0xFFFF) {
        out->valid = false;
        ESP_LOGE(TAG, "Invalid length field. ");
        return false; // ошибка длины
    }

    if (length_to_copy > PYLON_MAX_DATA_BYTES - index - 3) {
        ESP_LOGE(TAG, "Length exceeds maximum data size");
        out->valid = false;
        return false; // длина превышает максимальный размер данных
    }

    out->data_length = copy_ascii_to_hex(&input[index], (char *) out->data, length_to_copy);
    index += length_to_copy;
    ESP_LOGD(TAG, "Copied %d bytes, index is %zu", out->data_length, index);

    uint16_t checksum = compute_checksum_ascii(&input[1], len_ascii - 6); // - SOI - EOI - checksum

    out->checksum = ascii_hex4_to_u16(&input[index]);
    out->valid = (out->checksum == checksum);
    ESP_LOGI(TAG, "Checksum: %04X, computed: %04X", out->checksum, checksum);
    if (!out->valid) {
        ESP_LOGE(TAG, "Checksum error");
        return false; // ошибка контрольной суммы
    }

    return out->valid;
}

bool pylon_parse_info_payload(const uint8_t *info, size_t len, PylonBatteryStatus *out) {
    if (!info || !out) {
        ESP_LOGE("pylon_flat", "Null pointer");
        return false;
    }

    ESP_LOG_BUFFER_HEXDUMP(TAG, info, len, ESP_LOG_DEBUG);

    size_t index = 0;

    if (index + 2 > len) return false;
    out->modules = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Module ID: %u", out->modules);

    if (index + 1 > len) return false;
    out->cell_count = pylon_read_u8(&info[index]);
    index += 1;
    ESP_LOGI(TAG, "Cells: %u", out->cell_count);

    if (out->cell_count > PYLON_MAX_CELLS) {
        ESP_LOGE("pylon_flat", "Cell count too large: %u", out->cell_count);
        return false;
    }

    for (int i = 0; i < out->cell_count; i++) {
        if (index + 2 > len) return false;
        out->cell_voltage_mV[i] = pylon_read_u16(&info[index]);
        index += 2;
        ESP_LOGI(TAG, "Cell %d: %u mV", i, out->cell_voltage_mV[i]);
    }

    if (index + 1 > len) return false;
    out->temperature_count = pylon_read_u8(&info[index]);
    index += 1;

    if (out->temperature_count > PYLON_MAX_TEMPS) {
        ESP_LOGE("pylon_flat", "Temperature count too large: %u", out->temperature_count);
        return false;
    }

    for (int i = 0; i < out->temperature_count; i++) {
        if (index + 2 > len) return false;
        out->temperatures_c[i] = pylon_read_s16(&info[index]);
        index += 2;
        ESP_LOGI(TAG, "Temperature %d: %d.%02d C", i, out->temperatures_c[i]/100, abs(out->temperatures_c[i] % 100));
    }

    if (index + 2 > len) return false;
    out->current_mA = pylon_read_s16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Current: %d mA", out->current_mA);
    if (out->current_mA > 0) {
        ESP_LOGI(TAG, "State: Charging");
    } else if (out->current_mA < 0) {
        ESP_LOGI(TAG, "State: Discharging");
    } else {
        ESP_LOGI(TAG, "State: Idle");
    }

    if (index + 2 > len) return false;
    out->total_voltage_mV = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Total voltage: %d.%02d mV", out->total_voltage_mV/1000, abs(out->total_voltage_mV % 1000));

    if (index + 2 > len) return false;
    out->remaining_capacity_ah = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Remaining capacity: %d.%02d Ah", out->remaining_capacity_ah/100, abs(out->remaining_capacity_ah % 100));

    if (index + 1 > len) return false;
    out->user_defined_number = pylon_read_u8(&info[index]);
    index += 1;
    ESP_LOGI(TAG, "User defined number: %u", out->user_defined_number);

    if (index + 2 > len) return false;
    out->total_capacity_ah = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Total capacity: %d.%02d Ah", out->total_capacity_ah / 100, abs(out->total_capacity_ah % 100));

    if (index + 2 > len) return false;
    out->cycle_count = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Cycle count: %u", out->cycle_count);

    if (index + 2 > len) return false;
    out->batteryCapacity = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Battery capacity: %d.%02d Ah", out->batteryCapacity / 100, abs(out->batteryCapacity % 100));

    if (index + 2 > len) return false;
    out->currentCapacity = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Current capacity: %d.%02d Ah", out->currentCapacity / 100, abs(out->currentCapacity % 100));

    if (index + 2 > len) return false;
    out->userVoltage0 = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "User voltage 0: %d.%02d mV", out->userVoltage0/ 1000, abs(out->userVoltage0 % 1000));

    if (index + 2 > len) return false;
    out->userVoltage1 = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "User voltage 1: %d.%02d mV", out->userVoltage1/ 1000, abs(out->userVoltage1 % 1000));

    if (index + 2 > len) return false;
    out->userVoltage2 = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "User voltage 2: %d.%02d mV", out->userVoltage2/ 1000, abs(out->userVoltage2 % 1000));

    if (index + 2 > len) return false;
    out->userVoltage3 = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "User voltage 3: %d.%02d mV", out->userVoltage3/ 1000, abs(out->userVoltage3 % 1000));

    if (index + 2 > len) return false;
    out->userVoltage4 = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "User voltage 4: %d.%02d mV", out->userVoltage4/ 1000, abs(out->userVoltage4 % 1000));

    if (index + 2 > len) return false;
    out->unknown = pylon_read_u16(&info[index]);
    index += 2;
    ESP_LOGI(TAG, "Unknown: %u", out->unknown);

    return true;
}

