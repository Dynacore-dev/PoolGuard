#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =====================================================================
//  Config.example.h – Vorlage für Config.h
//  Kopieren nach Config.h und die WLAN-Zugangsdaten eintragen.
//  Config.h selbst ist in .gitignore und wird NICHT versioniert,
//  damit WLAN-Zugangsdaten nicht versehentlich geteilt werden.
// =====================================================================

struct TimeRange {
  int startH;
  int startM;
  int endH;
  int endM;
};

namespace Config {

// ---------------------- WLAN ------------------------------------------
constexpr const char* WIFI_SSID     = "DEIN_WLAN_NAME";
constexpr const char* WIFI_PASSWORD = "DEIN_WLAN_PASSWORT";
constexpr const char* WIFI_HOSTNAME = "esp32sd";

// ---------------------- Webserver ---------------------------------------
constexpr uint16_t SERVER_PORT = 80;

// ---------------------- NTP / Zeit ---------------------------------------
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr const char* TIMEZONE   = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr unsigned long TIME_SYNC_INTERVAL_MS = 24UL * 60 * 60 * 1000;  // 1x pro Tag

// ---------------------- Pins ---------------------------------------------
constexpr int PIN_TEMP_SENSOR = 16;  // DS18B20 (OneWire)
constexpr int PIN_POOL_PUMP   = 32;
constexpr int PIN_PH_PUMP     = 25;
constexpr int PIN_CL_PUMP     = 27;

// ---------------------- Pumpen-Zeitpläne ----------------------------------
// Poolpumpe: bis zu 3 Zeitfenster (Start/Ende)
constexpr TimeRange POOL_INTERVALS[3] = {
  { 8, 0, 10, 0 },   // 08:00 - 10:00
  { 14, 0, 16, 0 },  // 14:00 - 16:00
  { 20, 0, 22, 0 },  // 20:00 - 22:00
};

// PH-/Chlor-Pumpe: nur Startzeit, Ende ergibt sich aus PH_DOSE_SEC/CL_DOSE_SEC
constexpr TimeRange PH_START = { 8, 10, 0, 0 };   // Start 08:10
constexpr TimeRange CL_START = { 20, 10, 0, 0 };  // Start 20:10

// ---------------------- Dosiermenge Chemie-Pumpen --------------------------
// Zeit/Menge linear, 440 Sek. = 1L
enum DoseDuration {
  DOSE_0125L = 55,   // 55 Sek.       = 0,125 L
  DOSE_025L = 110,   // 1:50 Min      = 0,25 L
  DOSE_05L = 220,    // 3:40 Min      = 0,5 L
};
// Hier je nach Bedarf umstellen: DOSE_0125L / DOSE_025L / DOSE_05L
constexpr int PH_DOSE_SEC = DOSE_0125L;
constexpr int CL_DOSE_SEC = DOSE_0125L;

// ---------------------- Weitere Zeit-/Intervalleinstellungen ----------------
constexpr unsigned long MAIN_LOOP_DELAY_MS          = 100;   // Verzögerung in loop()
constexpr int            WIFI_CONNECT_MAX_ATTEMPTS   = 30;    // Versuche beim WLAN-Verbindungsaufbau
constexpr unsigned long WIFI_CONNECT_RETRY_DELAY_MS = 500;   // Wartezeit zwischen den Versuchen
constexpr unsigned long TEMP_SENSOR_INTERVAL_MS     = 2000;  // Messintervall DS18B20

}  // namespace Config

#endif  // CONFIG_H
