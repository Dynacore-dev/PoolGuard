#include "WifiManager.hpp"

WifiManager::WifiManager(const char* ssid, const char* password, const char* host)
  : _ssid(ssid), _password(password), _host(host), connectionCount(0) {}

void WifiManager::begin() {
  esp_wifi_set_ps(WIFI_PS_NONE);  // Deaktiviert WiFi Power Save
  Serial.printf("Verbinde mit %s ", _ssid);
  WiFi.begin(_ssid, _password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
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
    // _server.begin(); // Startet den WiFiServer
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

void wifi_disconnect() {
  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}