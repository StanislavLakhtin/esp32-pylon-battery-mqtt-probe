// time_sync.h
#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdbool.h>
#include <stddef.h>

#define NTP_SERVER CONFIG_PROBE_NTP_SERVER

void time_sync_init(void);

bool get_current_iso8601(char *buf, size_t maxlen);

#endif // TIME_SYNC_H
