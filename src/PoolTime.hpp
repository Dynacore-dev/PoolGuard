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

  // query individual time values
  int getHour();
  int getMinute();
  int getSecond();

  String getFormattedTime();
};

#endif
