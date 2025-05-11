/**
 * The software was developed by Stanislav Lakhtin (lakhtin.stanislav@gmail.com)
 * You can use it or parts of it freely in any non-commercial projects.
 * In case of use in commercial projects, please kindly contact the author.
 */

#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>
#include <sys/time.h>

#define NTP_SERVER CONFIG_PROBE_NTP_SERVER

void time_sync_init(void);

bool get_current_iso8601(char *buf, size_t maxlen);
time_t get_now(void);

#endif // TIME_SYNC_H
