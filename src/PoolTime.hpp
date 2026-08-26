#ifndef POOL_TIME_HPP
#define POOL_TIME_HPP

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "Config.h"

class DailyTimeSync {
private:
  const char* _ntpServer;
  const char* _timezone;
  unsigned long _lastSyncMillis;
  const unsigned long _syncInterval = Config::TIME_SYNC_INTERVAL_MS;

public:
  DailyTimeSync(const char* server = Config::NTP_SERVER,
                const char* tz = Config::TIMEZONE);

  void begin();
  void sync();
  void updateIfNeeded();

  // Einzelne Zeitwerte abfragen
  int getHour();
  int getMinute();
  int getSecond();

  String getFormattedTime();
};

class PoolPumpTime {
private:

public:

  bool checkPoolpumpTime(int switchTimeOnHour1, int switchTimeOffHour1, int switchTimeOnHour2,
                         int switchTimeOffHour2, int switchTimeOnHour3, int switchTimeOffHour3);

  bool checkPHpumpTime(int switchTimeOnHour, int switchTimeOffHour, int switchTimeOnMinute,
                       int switchTimeOffMinute, int switchTimeOnSec, int switchTimeOffSec);

  bool check_Chlor_pump_status(int switchTimeOnHour, int switchTimeOffHour, int switchTimeOnMinute,
                               int switchTimeOffMinute, int switchTimeOnSec, int switchTimeOffSec);
};

#endif
