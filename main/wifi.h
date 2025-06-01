/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */
#ifndef WIFI_H
#define WIFI_H

#include "mqtt_client.h"

void wifi_init_sta(esp_mqtt_client_handle_t mqtt_client);
#endif
