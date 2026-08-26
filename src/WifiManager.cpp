#include "WifiManager.hpp"
#include "Config.h"

WifiManager::WifiManager(const char* ssid, const char* password, const char* host)
  : _ssid(ssid), _password(password), _host(host), connectionCount(0) {}

void WifiManager::begin() {
  esp_wifi_set_ps(WIFI_PS_NONE);  // Deaktiviert WiFi Power Save
  Serial.printf("Verbinde mit %s ", _ssid);
  WiFi.begin(_ssid, _password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < Config::WIFI_CONNECT_MAX_ATTEMPTS) {
    delay(Config::WIFI_CONNECT_RETRY_DELAY_MS);
    Serial.print(".");
    attempts++;
  }

  if (isConnected()) {
    Serial.println("Connected to the WiFi network");
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nVerbindung fehlgeschlagen.");
  }
}

bool WifiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIP() {
  return WiFi.localIP().toString();
}