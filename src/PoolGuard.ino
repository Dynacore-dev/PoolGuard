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

void setup() {
  Serial.begin(115200);
  // wait for serial connection
  while (!Serial)
    ;
  Serial.println("PoolGuard gestartet");

  esp_task_wdt_init(Config::WATCHDOG_TIMEOUT_SEC, true);  // reboot if loop() ever gets stuck
  esp_task_wdt_add(NULL);

  wifi.begin();
  if (wifi.isConnected()) {
    // 2. start time sync
    rtcClock.begin();
    // 3. start server
    myServer.begin(&rtcClock);

    if (Config::OTA_ENABLED) {
      ArduinoOTA.setHostname(Config::WIFI_HOSTNAME);
      ArduinoOTA.setPassword(Config::OTA_PASSWORD);
      ArduinoOTA.begin();
    }
  }
}

void loop() {
  esp_task_wdt_reset();

  if (!wifi.isConnected()) {
    wifi.begin();
    Serial.print(String("Wlan reconnected - count: ") + wifi.connectionCount);
    wifi.connectionCount++;
  }

  if (Config::OTA_ENABLED) ArduinoOTA.handle();

  rtcClock.updateIfNeeded();
  myServer.handleLogic();  // controls all 3 pumps based on mode/time
  delay(Config::MAIN_LOOP_DELAY_MS);
}
