/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */

#ifndef MQTT_FORMATTER_H
#define MQTT_FORMATTER_H

#include "mqtt_queue.h"
#include "pylon_packet.h"
#include <stdbool.h>

bool mqtt_format_info_payload(const PylonBatteryStatus *status, MQTTPayload *out);

#endif
