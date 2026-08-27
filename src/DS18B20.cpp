#include "DS18B20.hpp"

DS18B20::DS18B20(int pin) : _oneWire(pin), _sensors(&_oneWire) {}

void DS18B20::begin() {
  _sensors.begin();
  _sensors.setWaitForConversion(false); // important: prevents the program from blocking

  // Start the first conversion here and delay update()'s first read by one
  // full _interval (via _lastMillis), giving the DS18B20 time to finish
  // converting. Without this, update()'s first read happens before any
  // conversion was ever requested and can return the sensor's 85.00C
  // power-on default instead of a real reading or a disconnect error.
  _sensors.requestTemperatures();
  _lastMillis = millis();
}

void DS18B20::update() {
  // check whether the interval has elapsed
  if (millis() - _lastMillis >= _interval) {
    _lastMillis = millis();

    // 1. fetch temperature from the previous request
    float temp = _sensors.getTempCByIndex(0);
    _sensorOk = (temp != DEVICE_DISCONNECTED_C);
    if (_sensorOk) {
      _lastTemp = temp;
    }

    // 2. start a new request for the next cycle
    _sensors.requestTemperatures();
  }
}

void DS18B20::printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void DS18B20::printTemperature(DeviceAddress deviceAddress) {
  float tempC = _sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED_C) {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.println(tempC);
}

void DS18B20::printResolution(DeviceAddress deviceAddress) {
  Serial.print("Resolution: ");
  Serial.println(_sensors.getResolution(deviceAddress));
}
