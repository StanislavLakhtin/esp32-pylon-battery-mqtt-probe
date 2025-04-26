/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#ifndef PYLON_PACKET_H
#define PYLON_PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PYLON_MAX_DATA_BYTES 2100
#define PYLON_MAX_CELLS 32
#define PYLON_MAX_TEMPS 8

#define PYLON_CHECK(expr, message, ...) \
    do { \
        if (!(expr)) { \
            ESP_LOGE("pylon_check", "[%s:%d] " message, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            return false; \
        } \
    } while (0)

typedef struct {
    uint8_t version; // версия протокола
    uint8_t address; // адрес устройства
    uint8_t cid1; // код команды
    uint8_t cid2; // код команды
    uint16_t data_length; // длина payload (как в протоколе)
    uint8_t data[PYLON_MAX_DATA_BYTES];
    uint16_t checksum; // XOR от version до последнего байта data
    bool valid; // флаг валидности после разбора
} PylonPacketRaw;

typedef struct {
    uint16_t modules; // количество параллельных батарей (модулей)
    uint8_t cell_count; // количество ячеек
    uint16_t cell_voltage_mV[PYLON_MAX_CELLS]; // массив ячеек (максимум 32)

    uint8_t temperature_count; // количество температурных датчиков
    int16_t temperatures_c[PYLON_MAX_TEMPS]; // температура в десятых градусах Цельсия

    int16_t current_mA; // ток заряда/разряда
    uint16_t total_voltage_mV; // общее напряжение

    uint16_t remaining_capacity_ah; // оставшаяся ёмкость
    uint8_t user_defined_number; // пользовательский номер

    uint16_t total_capacity_ah; // общая ёмкость
    uint16_t cycle_count; // число циклов

    // HexWord cycles;
    uint16_t batteryCapacity;
    uint16_t currentCapacity;
    uint16_t userVoltage0;
    uint16_t userVoltage1;
    uint16_t userVoltage2;
    uint16_t userVoltage3;
    uint16_t userVoltage4;
    uint16_t unknown;
} PylonBatteryStatus;

bool pylon_decode_ascii_hex(const char *input, size_t len_ascii, PylonPacketRaw *out);

bool pylon_parse_info_payload(const uint8_t *info, size_t len, PylonBatteryStatus *out);

/**
 * Read 8-bit unsigned integer
 */
static inline uint8_t pylon_read_u8(const uint8_t *data) {
    return data[0];
}

/**
 * Read 16-bit unsigned integer, high byte first (Pylon protocol format)
 */
static inline uint16_t pylon_read_u16(const uint8_t *data) {
    return ((uint16_t)data[0] << 8) | data[1];
}

/**
 * Read 16-bit signed integer, high byte first (Pylon protocol format)
 */
static inline int16_t pylon_read_s16(const uint8_t *data) {
    return (int16_t)(((uint16_t)data[0] << 8) | data[1]);
}

#endif // PYLON_PACKET_H
