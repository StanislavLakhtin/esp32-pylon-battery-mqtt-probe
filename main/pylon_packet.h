#ifndef PYLON_PACKET_H
#define PYLON_PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PYLON_MAX_DATA_BYTES 2100
#define PYLON_MAX_CELLS 32
#define PYLON_MAX_TEMPS 8

typedef struct {
    uint8_t version; // версия протокола
    uint8_t address; // адрес устройства
    uint8_t cid1; // код команды
    uint8_t cid2; // код команды
    uint16_t length; // длина payload (как в протоколе)
    uint8_t data[PYLON_MAX_DATA_BYTES];
    uint8_t checksum; // XOR от version до последнего байта data
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

#endif // PYLON_PACKET_H
