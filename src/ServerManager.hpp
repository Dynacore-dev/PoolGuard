#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "Config.h"
#include "PoolTime.hpp"
#include "DS18B20.hpp"

enum DeviceMode { MODE_OFF = 0,
                  MODE_ON = 1,
                  MODE_AUTO = 2 };

struct Device {
  int pin;
  DeviceMode mode;
  TimeRange times[3];   // Platz für bis zu 3 Intervalle
  int activeIntervals;  // Wie viele Intervalle genutzt werden
};

class ServerManager {
private:
  AsyncWebServer _server;
  DailyTimeSync* _timeSync;  // Zeiger auf die Zeit-
  
  DS18B20 tempSensor;

  // Definition der drei Geräte - Zeitpläne & Dosiermenge kommen aus Config.h
  Device _pool = { Config::PIN_POOL_PUMP, MODE_AUTO,
                    { Config::POOL_INTERVALS[0], Config::POOL_INTERVALS[1], Config::POOL_INTERVALS[2] }, 3 };
  Device _ph = { Config::PIN_PH_PUMP, MODE_AUTO, { Config::PH_START }, 1 };
  Device _cl = { Config::PIN_CL_PUMP, MODE_AUTO, { Config::CL_START }, 1 };

  int _phDoseSec = Config::PH_DOSE_SEC;
  int _clDoseSec = Config::CL_DOSE_SEC;

  static String formatDoseLabel(int durationSec);

  bool isInside(int h, int m, int s, TimeRange tr, int durationSec = -1);

public:
  ServerManager(int port = Config::SERVER_PORT);
  void begin(DailyTimeSync* ts);
  void handleLogic();  // Prüft die Zeit im Auto-Modus
  bool isPoolpumpActiv;
};

#endif
