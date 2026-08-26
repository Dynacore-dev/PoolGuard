#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =====================================================================
//  Config.example.h – template for Config.h
//  Copy to Config.h and fill in the WiFi credentials.
//  Config.h itself is in .gitignore and is NOT version-controlled,
//  so WiFi credentials aren't accidentally shared.
// =====================================================================

struct TimeRange {
  int startH;
  int startM;
  int endH;
  int endM;
};

namespace Config {

// ---------------------- WiFi ------------------------------------------
constexpr const char* WIFI_SSID     = "DEIN_WLAN_NAME";
constexpr const char* WIFI_PASSWORD = "DEIN_WLAN_PASSWORT";
constexpr const char* WIFI_HOSTNAME = "esp32sd";

// ---------------------- Web server ---------------------------------------
constexpr uint16_t SERVER_PORT = 80;

// ---------------------- NTP / time ---------------------------------------
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr const char* TIMEZONE   = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr unsigned long TIME_SYNC_INTERVAL_MS = 24UL * 60 * 60 * 1000;  // once per day

// ---------------------- Pins ---------------------------------------------
constexpr int PIN_TEMP_SENSOR = 16;  // DS18B20 (OneWire)
constexpr int PIN_POOL_PUMP   = 32;
constexpr int PIN_PH_PUMP     = 25;
constexpr int PIN_CL_PUMP     = 27;

// ---------------------- Pump schedules ----------------------------------
// Pool pump: up to 3 time windows (start/end)
constexpr TimeRange POOL_INTERVALS[3] = {
  { 8, 0, 10, 0 },   // 08:00 - 10:00
  { 14, 0, 16, 0 },  // 14:00 - 16:00
  { 20, 0, 22, 0 },  // 20:00 - 22:00
};

// pH/chlorine pump: start time only, end results from PH_DOSE_SEC/CL_DOSE_SEC
constexpr TimeRange PH_START = { 8, 10, 0, 0 };   // starts 08:10
constexpr TimeRange CL_START = { 20, 10, 0, 0 };  // starts 20:10

// ---------------------- Dosing amount, chemical pumps --------------------------
// time/amount linear, 440 sec = 1L
enum DoseDuration {
  DOSE_0125L = 55,   // 55 sec        = 0,125 L
  DOSE_025L = 110,   // 1:50 min      = 0,25 L
  DOSE_05L = 220,    // 3:40 min      = 0,5 L
};
// switch here as needed: DOSE_0125L / DOSE_025L / DOSE_05L
constexpr int PH_DOSE_SEC = DOSE_0125L;
constexpr int CL_DOSE_SEC = DOSE_0125L;

// ---------------------- Dashboard language --------------------------------
enum class Language { DE, EN };
constexpr Language DASHBOARD_LANGUAGE = Language::DE;  // Language::DE or Language::EN

// ---------------------- Dashboard authentication ---------------------------
// HTTP Basic Auth for all dashboard/control endpoints (/, /set, /status).
// Off by default (trusted home network); set to true to require a login.
constexpr bool WEB_AUTH_ENABLED = false;
constexpr const char* WEB_AUTH_USER     = "admin";
constexpr const char* WEB_AUTH_PASSWORD = "DEIN_DASHBOARD_PASSWORT";

// ---------------------- Other time/interval settings ----------------
constexpr unsigned long MAIN_LOOP_DELAY_MS          = 100;   // delay in loop()
constexpr int            WIFI_CONNECT_MAX_ATTEMPTS   = 30;    // attempts when connecting to WiFi
constexpr unsigned long WIFI_CONNECT_RETRY_DELAY_MS = 500;   // wait time between attempts
constexpr unsigned long TEMP_SENSOR_INTERVAL_MS     = 2000;  // DS18B20 measurement interval

}  // namespace Config

#endif  // CONFIG_H
