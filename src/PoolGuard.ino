#include <dummy.h>
#include <esp_task_wdt.h>
#include <ArduinoOTA.h>
#include "Config.h"
#include "PoolTime.hpp"
#include "WifiManager.hpp"
#include "ServerManager.hpp"

DailyTimeSync rtcClock(Config::NTP_SERVER, Config::TIMEZONE);
WifiManager wifi(Config::WIFI_SSID, Config::WIFI_PASSWORD, Config::WIFI_HOSTNAME);
ServerManager myServer(Config::SERVER_PORT);

bool otaStarted = false;

// ArduinoOTA.begin() also registers the mDNS responder for
// WIFI_HOSTNAME.local. Only call this once WiFi is actually connected -
// starting it while disconnected leaves both OTA and mDNS dead until the
// next reboot, since begin() is never retried on its own.
void beginOTA() {
  ArduinoOTA.setHostname(Config::WIFI_HOSTNAME);
  ArduinoOTA.setPassword(Config::OTA_PASSWORD);
  ArduinoOTA.begin();
  otaStarted = true;
}

void setup() {
  Serial.begin(115200);
  // wait for serial connection
  while (!Serial)
    ;
  Serial.println("PoolGuard gestartet");

  esp_task_wdt_init(Config::WATCHDOG_TIMEOUT_SEC, true);  // reboot if loop() ever gets stuck
  esp_task_wdt_add(NULL);

  wifi.begin();

  // Initialize regardless of whether the initial WiFi connection attempt
  // succeeded: loop() keeps retrying WiFi afterward, but handleLogic() runs
  // on every loop() tick and needs _timeSync set and the pump pins
  // configured. Gating this on wifi.isConnected() left _timeSync null (and
  // pins/server/OTA never set up) whenever WiFi wasn't already up at boot,
  // with no re-initialization once it reconnected.
  rtcClock.begin();
  myServer.begin(&rtcClock);

  if (Config::OTA_ENABLED && wifi.isConnected()) {
    beginOTA();
  }
}

void loop() {
  esp_task_wdt_reset();

  if (!wifi.isConnected()) {
    wifi.begin();
    Serial.print(String("Wlan reconnected - count: ") + wifi.connectionCount);
    wifi.connectionCount++;
  }

  if (Config::OTA_ENABLED) {
    if (!otaStarted && wifi.isConnected()) beginOTA();
    if (otaStarted) ArduinoOTA.handle();
  }

  rtcClock.updateIfNeeded();
  myServer.handleLogic();  // controls all 3 pumps based on mode/time
  delay(Config::MAIN_LOOP_DELAY_MS);
}
