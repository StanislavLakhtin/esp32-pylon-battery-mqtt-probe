#ifndef PYLON_PACKET_H
#define PYLON_PACKET_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PYLON_MAX_DATA_BYTES 2100

typedef struct {
    uint8_t version;        // версия протокола
    uint8_t address;        // адрес устройства
    uint8_t cid;            // скомбинированный код команды (например, 'F1' → 0xF1)
    uint16_t length;        // длина payload (как в протоколе)
    uint8_t data[PYLON_MAX_DATA_BYTES];
    uint8_t checksum;       // XOR от version до последнего байта data
    bool valid;             // флаг валидности после разбора
} PylonPacketRaw;

bool pylon_decode_ascii_hex(const char *input, size_t len_ascii, PylonPacketRaw *out);

#endif // PYLON_PACKET_H
