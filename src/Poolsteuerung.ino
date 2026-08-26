#include <dummy.h>
#include "PoolTime.hpp"
#include "WifiManager.hpp"
#include "ServerManager.hpp"

const char* ssid = "REDACTED_SSID";
const char* password = "REDACTED_PASSWORD";
const char* host = "esp32sd";

DailyTimeSync rtcClock;
WifiManager wifi(ssid, password, host);
ServerManager myServer(80);  // Webserver auf Port 80

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
  delay(100);
}
