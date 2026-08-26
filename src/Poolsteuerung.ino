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
  // auf serielle Verbindung warten
  while (!Serial)
    ;
  Serial.println("Poolsteuerung gestartet");
  wifi.begin();
  rtcClock.begin();
  if (wifi.isConnected()) {
    // 2. Zeit-Sync starten
    rtcClock.begin();
    // 3. Server starten
    myServer.begin(&rtcClock);
  }
}

void loop() {
  if (!wifi.isConnected()) {
    wifi.begin();
    Serial.print("Wlan reconnected - count: " + wifi.connectionCount);
    wifi.connectionCount++;
  }

  rtcClock.updateIfNeeded();
  myServer.handleLogic();  // Steuert alle 3 Pumpen basierend auf Modus/Zeit
  delay(Config::MAIN_LOOP_DELAY_MS);
}
