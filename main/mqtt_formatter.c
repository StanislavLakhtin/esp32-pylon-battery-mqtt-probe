#include "mqtt_formatter.h"
#include "time_sync.h"
#include <stdio.h>
#include <string.h>

bool mqtt_format_info_payload(const PylonBatteryStatus *s, MQTTPayload *out) {
    if (!s || !out) return false;

    char timestamp[32];
    if (!get_current_iso8601(timestamp, sizeof(timestamp))) {
        strncpy(timestamp, "unknown", sizeof(timestamp));
    }

    snprintf(out->topic, sizeof(out->topic), "/esp32/battery/%02X/info", s->user_defined_number);
    out->qos = 1;
    out->retain = 0;

    char cell_voltages[512] = {0};
    strcat(cell_voltages, "[");
    for (int i = 0; i < s->cell_count; ++i) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%u%s", s->cell_voltage_mV[i], i < s->cell_count - 1 ? "," : "");
        strncat(cell_voltages, buf, sizeof(cell_voltages) - strlen(cell_voltages) - 1);
    }
    strcat(cell_voltages, "]");

    char temps[256] = {0};
    strcat(temps, "[");
    for (int i = 0; i < s->temperature_count; ++i) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d%s", s->temperatures_c[i], i < s->temperature_count - 1 ? "," : "");
        strncat(temps, buf, sizeof(temps) - strlen(temps) - 1);
    }
    strcat(temps, "]");

    int len = snprintf(out->payload, sizeof(out->payload),
                       "{\"timestamp\":\"%s\",\"modules\":%u,\"cell_count\":%u,\"temperature_count\":%u,"
                       "\"cell_voltage_mV\":%s,\"temperatures_c\":%s,"
                       "\"current_mA\":%d,\"total_voltage_mV\":%u,\"remaining_capacity_ah\":%u,"
                       "\"user_defined_number\":%u,\"total_capacity_ah\":%u,\"cycle_count\":%u,"
                       "\"batteryCapacity\":%u,\"currentCapacity\":%u,\"userVoltage0\":%u,\""
                       "userVoltage1\":%u,\"userVoltage2\":%u,\"userVoltage3\":%u,\"userVoltage4\":%u,\""
                       "unknown\":%u}",
                       timestamp,
                       s->modules,
                       s->cell_count,
                       s->temperature_count,
                       cell_voltages,
                       temps,
                       s->current_mA,
                       s->total_voltage_mV,
                       s->remaining_capacity_ah,
                       s->user_defined_number,
                       s->total_capacity_ah,
                       s->cycle_count,
                       s->batteryCapacity,
                       s->currentCapacity,
                       s->userVoltage0,
                       s->userVoltage1,
                       s->userVoltage2,
                       s->userVoltage3,
                       s->userVoltage4,
                       s->unknown
    );

    return len > 0 && len < sizeof(out->payload);
}
