#ifndef WIFI_MANAGER_HPP
#define WIFI_MANAGER_HPP

#include <WiFi.h>
#include <esp_wifi.h>

class WifiManager {
private:
    const char* _ssid;
    const char* _password;
    const char* _host;

public:
    WifiManager(const char* ssid, const char* password, const char* host);

    void begin();
    bool isConnected();

    // helper for the IP address
    String getIP();

    int connectionCount;
};

#endif
