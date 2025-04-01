#ifndef MQTT_FORMATTER_H
#define MQTT_FORMATTER_H

#include "mqtt_queue.h"
#include "pylon_packet.h"
#include <stdbool.h>

bool mqtt_format_info_payload(const PylonBatteryStatus *status, MQTTPayload *out);

#endif
