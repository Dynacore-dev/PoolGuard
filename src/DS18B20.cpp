#include "DS18B20.hpp"

DS18B20::DS18B20(int pin) : _oneWire(pin), _sensors(&_oneWire) {}

void DS18B20::begin() {
  _sensors.begin();
  _sensors.setWaitForConversion(false); // Wichtig: Verhindert, dass das Programm blockiert
}

void DS18B20::update() {
  // Prüfen, ob das Intervall abgelaufen ist
  if (millis() - _lastMillis >= _interval) {
    _lastMillis = millis();
    
    // 1. Temperatur vom vorherigen Request abholen
    float temp = _sensors.getTempCByIndex(0);
    if (temp != DEVICE_DISCONNECTED_C) {
      _lastTemp = temp;
    }

    // 2. Neuen Request für den nächsten Zyklus starten
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
