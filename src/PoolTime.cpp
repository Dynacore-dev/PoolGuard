#include "PoolTime.hpp"

DailyTimeSync::DailyTimeSync(const char* server, const char* tz) 
    : _ntpServer(server), _timezone(tz), _lastSyncMillis(0) {}

void DailyTimeSync::begin() {
    configTzTime(_timezone, _ntpServer);
    sync();
}

void DailyTimeSync::sync() {
    if (WiFi.status() == WL_CONNECTED) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo, 10000)) { 
            _lastSyncMillis = millis();
        }
    }
}

void DailyTimeSync::updateIfNeeded() {
    if (millis() - _lastSyncMillis >= _syncInterval) {
        sync();
    }
}

// helper method: gets the current time structure from the internal RTC
int DailyTimeSync::getHour() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return 0;
    return timeinfo.tm_hour;
}

int DailyTimeSync::getMinute() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return 0;
    return timeinfo.tm_min;
}

int DailyTimeSync::getSecond() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return 0;
    return timeinfo.tm_sec;
}

String DailyTimeSync::getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "Zeit nicht gesetzt";
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
    return String(buffer);
}
