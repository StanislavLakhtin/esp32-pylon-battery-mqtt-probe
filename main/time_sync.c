#include "time_sync.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "time_sync";

#ifndef PROBE_UTC_OFFSET_MINUTES
#define PROBE_UTC_OFFSET_MINUTES CONFIG_PROBE_NTP_UTC_OFFSET_MINUTES
#endif

static bool synced = false;

static void time_sync_notification_cb(struct timeval *tv) {
    ESP_LOGI(TAG, "Time synchronized via SNTP");
    synced = true;
}

void time_sync_init(void) {
    int16_t time_shift = PROBE_UTC_OFFSET_MINUTES;
    ESP_LOGI(TAG, "Initializing SNTP (UTC offset %+d min)", time_shift);
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

bool get_current_iso8601(char *buf, size_t maxlen) {
    if (!buf || maxlen < 25) return false; // requires "YYYY-MM-DDTHH:MM:SS±HH:MM"

    time_t now = 0;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    // apply UTC offset from config
    int offset_sec = PROBE_UTC_OFFSET_MINUTES * 60;
    now += offset_sec;
    gmtime_r(&now, &timeinfo);

    int offset_min = PROBE_UTC_OFFSET_MINUTES;
    int hours = offset_min / 60;
    int minutes = abs(offset_min % 60);

    snprintf(buf, maxlen, "%04d-%02d-%02dT%02d:%02d:%02d%+03d:%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec,
             hours,
             minutes);

    return true;
}
