#ifndef SERVER_MANAGER_HPP
#define SERVER_MANAGER_HPP

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "PoolTime.hpp"
#include "DS18B20.hpp"

enum DeviceMode { MODE_OFF = 0,
                  MODE_ON = 1,
                  MODE_AUTO = 2 };

// Dosiermenge der Chemie-Pumpen (Zeit/Menge linear, 440 Sek. = 1L):
enum DoseDuration {
  DOSE_0125L = 55,   // 55 Sek.       = 0,125 L
  DOSE_025L = 110,   // 1:50 Min      = 0,25 L
  DOSE_05L = 220,    // 3:40 Min      = 0,5 L
};

struct TimeRange {
  int startH;
  int startM;
  int endH;
  int endM;
};

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

  // Definition der drei Geräte
  Device _pool = { 32, MODE_AUTO, { { 8, 0, 10, 0 }, { 14, 0, 16, 0 }, { 20, 0, 22, 0 } }, 3 };  // 08-10 / 14-16 / 20-22 Uhr
  // Alt (fehlerhaft): die "50" landete als startH von times[1] statt als Dauer - Dauer wird separat als durationSec an isInside() übergeben.
  // Device _ph = { 25, MODE_AUTO, { { 8, 10, 8, 11 }, 50 }, 1 };
  // Device _cl = { 27, MODE_AUTO, { { 20, 10, 20, 11 }, 50 }, 1 };
  Device _ph = { 25, MODE_AUTO, { { 8, 10, 8, 11 } }, 1 };    // Start 08:10
  Device _cl = { 27, MODE_AUTO, { { 20, 10, 20, 11 } }, 1 };  // Start 20:10

  // Hier je nach Bedarf umstellen: DOSE_0125L / DOSE_025L / DOSE_05L
  int _phDoseSec = DOSE_0125L;
  int _clDoseSec = DOSE_0125L;

  static String formatDoseLabel(int durationSec);

  bool isInside(int h, int m, int s, TimeRange tr, int durationSec = -1);

  void updateDevice(Device& d);

public:
  ServerManager(int port = 80);
  void begin(DailyTimeSync* ts);
  void handleLogic();  // Prüft die Zeit im Auto-Modus
  bool isPoolpumpActiv;
};

#endif
