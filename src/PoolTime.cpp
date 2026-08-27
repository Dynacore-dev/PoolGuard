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

// getLocalTime() returns immediately once the RTC has a valid time, so a
// short timeout here doesn't delay the normal case - it only bounds the
// worst case (time never synced) to a few ms instead of the 5000ms default,
// since these getters are called every loop() tick via handleLogic().
static const uint32_t GET_TIME_TIMEOUT_MS = 5;

// helper method: gets the current time structure from the internal RTC
int DailyTimeSync::getHour() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, GET_TIME_TIMEOUT_MS)) return 0;
    return timeinfo.tm_hour;
}

int DailyTimeSync::getMinute() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, GET_TIME_TIMEOUT_MS)) return 0;
    return timeinfo.tm_min;
}

int DailyTimeSync::getSecond() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, GET_TIME_TIMEOUT_MS)) return 0;
    return timeinfo.tm_sec;
}

String DailyTimeSync::getFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo, GET_TIME_TIMEOUT_MS)) {
        return Config::DASHBOARD_LANGUAGE == Config::Language::EN ? "Time not set" : "Zeit nicht gesetzt";
    }
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &timeinfo);
    return String(buffer);
}
