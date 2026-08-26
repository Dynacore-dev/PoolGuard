# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

PlatformIO/Arduino firmware for an ESP32 (`esp32dev` board) that controls a pool's filter pump plus two dosing pumps (pH, chlorine) on a schedule, exposes a small web dashboard, and reads water temperature from a DS18B20 sensor.

## Setup

`src/Config.h` is gitignored (it holds real WiFi credentials) and must exist locally before building: copy `src/Config.example.h` to `src/Config.h` and fill in `Config::WIFI_SSID`/`WIFI_PASSWORD`. `pio run` will fail with `fatal error: Config.h: No such file or directory` if it's missing.

## Commands

- Build: `pio run`
- Upload to a connected board: `pio run -t upload`
- Serial monitor: `pio device monitor` (baud 115200, set in `Poolsteuerung.ino`)
- Clean build artifacts: `pio run -t clean`

There is no test suite (`test/` only holds PlatformIO's default scaffolding README) and no linter configured. `pio run` is the only correctness check available — always run it after changes, since this is embedded C++ with no other feedback loop.

Library dependencies are declared via `lib_deps` in `platformio.ini` and auto-install on `pio run`: `OneWire`, `DallasTemperature`, and the `esp32async` forks of `AsyncTCP`/`ESPAsyncWebServer` (the original `me-no-dev` packages are unmaintained — keep using the `esp32async` ones).

## Architecture

- **`Config.h`** is the single source of truth for everything environment/hardware-specific: WiFi credentials, web server port, NTP server/timezone/sync interval, GPIO pin assignments, the pool pump's time windows (`POOL_INTERVALS`), the pH/chlorine pumps' start times, dosing duration (`PH_DOSE_SEC`/`CL_DOSE_SEC`, chosen from the `DoseDuration` enum), WiFi reconnect behavior, main loop delay, and the temperature sensor's polling interval. New magic numbers should go here, not inline in the other files.
- **`Poolsteuerung.ino`** is the entry point: constructs `DailyTimeSync`, `WifiManager`, and `ServerManager` from `Config` values, then in `loop()` reconnects WiFi if dropped, re-syncs time if needed, and calls `ServerManager::handleLogic()` every tick.
- **`ServerManager`** is the core controller:
  - Holds three `Device` structs (`_pool`, `_ph`, `_cl`), each with a pin, a `DeviceMode` (`OFF`/`ON`/`AUTO`), and up to 3 `TimeRange` windows.
  - `handleLogic()` (called every `loop()` iteration) evaluates each device's mode against the current time via `isInside()` and drives the GPIO pins. **Safety interlock**: the pH/chlorine pumps only run in `AUTO`/`ON` mode if the pool pump is confirmed actually running (`flowOk = digitalRead(_pool.pin)`) — this prevents dosing chemicals into a stagnant pool.
  - Also owns the `AsyncWebServer` and the `DS18B20` temperature sensor; `handleLogic()` calls `tempSensor.update()` each tick (the sensor itself throttles actual reads via `Config::TEMP_SENSOR_INTERVAL_MS`).
  - Web routes are registered in `begin()`: `GET /` returns a self-contained HTML/CSS/JS dashboard (built as inline C++ string concatenation), `GET /set?dev=<pool|ph|cl>&m=<0|1|2>` changes a device's mode, `GET /status` returns JSON state that the dashboard polls every second.
  - Dosing amounts are shown to the user via `formatDoseLabel()`, which converts a duration in seconds to a liters figure using a linear model (440 sec = 1 L).
- **`WifiManager`** wraps STA-mode connection setup/retry and exposes `isConnected()`/`getIP()`.
- **`PoolTime` (`DailyTimeSync`)** syncs the ESP32's RTC via NTP on `begin()` and re-syncs once per `Config::TIME_SYNC_INTERVAL_MS`; `ServerManager` reads the current hour/minute/second from it for schedule checks rather than tracking time itself.
- **`DS18B20`** wraps `OneWire` + `DallasTemperature` for non-blocking temperature reads (`setWaitForConversion(false)`); `update()` must be called regularly (currently from `ServerManager::handleLogic()`) or the temperature never refreshes.
