#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include <WiFi.h>
#include <esp_wifi.h>

class WifiManager {
private:
    const char* _ssid;
    const char* _password;
    const char* _host;
    // WiFiServer _server; // Der Server-Instanz

public:
    // Konstruktor mit Port-Vorgabe (Standard 80)
    WifiManager(const char* ssid, const char* password, const char* host);

    void begin();
    bool isConnected();
    
    // Gibt ein Client-Objekt zurück, wenn eine Verbindung zum Server besteht
    WiFiClient availableClient();
    
    // Hilfsfunktion für die IP
    String getIP();

    int connectionCount;
};

#endif
