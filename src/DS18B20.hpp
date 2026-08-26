#ifndef DS18B20_HPP
#define DS18B20_HPP

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Config.h"

class DS18B20 {
  public:
    DS18B20(int pin);
    void begin();
    void update(); // processes measurements in the background
    
    float getLatestTemperature() { return _lastTemp; }
    void printAddress(DeviceAddress deviceAddress);
    void printTemperature(DeviceAddress deviceAddress);
    void printResolution(DeviceAddress deviceAddress);

  private:
    OneWire _oneWire;
    DallasTemperature _sensors;
    float _lastTemp = -127.0;
    unsigned long _lastMillis = 0;
    const unsigned long _interval = Config::TEMP_SENSOR_INTERVAL_MS;
};

#endif
