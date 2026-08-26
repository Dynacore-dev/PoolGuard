#include <dummy.h>
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
  wifi.begin();
  rtcClock.begin();
  if (wifi.isConnected()) {
    // 2. start time sync
    rtcClock.begin();
    // 3. start server
    myServer.begin(&rtcClock);
  }
}

void loop() {
  if (!wifi.isConnected()) {
    wifi.begin();
    Serial.print(String("Wlan reconnected - count: ") + wifi.connectionCount);
    wifi.connectionCount++;
  }

  rtcClock.updateIfNeeded();
  myServer.handleLogic();  // controls all 3 pumps based on mode/time
  delay(Config::MAIN_LOOP_DELAY_MS);
}
