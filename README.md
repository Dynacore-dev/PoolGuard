# PoolGuard

PlatformIO/Arduino firmware for an ESP32 that controls a pool's filter pump plus two dosing pumps (pH, chlorine) on a schedule, exposes a small web dashboard, and reads water temperature from a DS18B20 sensor.

## Features

- **Scheduled pool pump** — up to 3 daily on/off time windows (`Config::POOL_INTERVALS`).
- **pH / chlorine dosing pumps** — start at a configured time and run for a configurable dose duration.
- **Safety interlock** — the pH/chlorine pumps only run in `AUTO`/`ON` mode if the pool pump is confirmed actually running, so chemicals are never dosed into a stagnant pool.
- **Web dashboard** — self-contained HTML/CSS/JS page (no external assets) served directly from the ESP32, polling `/status` every second. Available in German or English (`Config::DASHBOARD_LANGUAGE`).
- **Manual override** — each device (pool, pH, chlorine) can be forced `OFF`/`ON` or left in `AUTO` (schedule-driven) via the dashboard.
- **Water temperature** — non-blocking DS18B20 readings, shown on the dashboard with a clear error state if the sensor is disconnected.
- **NTP time sync** — daily re-sync so schedules stay accurate without a RTC battery.
- **Optional dashboard auth** — HTTP Basic Auth for `/`, `/set`, and `/status`, off by default for use on a trusted home LAN. The server is plain HTTP (no TLS), so credentials travel unencrypted — only enable this on a network you trust.
- **OTA updates** — password-protected `ArduinoOTA`, enabled by default.
- **Watchdog** — reboots the device if the main loop ever hangs.

## Hardware

| Function          | Config constant   | Notes                          |
|--------------------|--------------------|---------------------------------|
| Temperature sensor  | `PIN_TEMP_SENSOR`  | DS18B20 via OneWire             |
| Pool filter pump    | `PIN_POOL_PUMP`    |                                  |
| pH dosing pump      | `PIN_PH_PUMP`      |                                  |
| Chlorine dosing pump| `PIN_CL_PUMP`      |                                  |

Pin numbers and all other hardware/environment specifics live in `src/Config.h` (see Setup below).

## Setup

`src/Config.h` is gitignored (it holds real WiFi credentials, the dashboard password, and the OTA password) and must exist locally before building:

```sh
cp src/Config.example.h src/Config.h
```

Then edit `src/Config.h` and fill in at least:

- `Config::WIFI_SSID` / `Config::WIFI_PASSWORD`
- `Config::WEB_AUTH_USER` / `Config::WEB_AUTH_PASSWORD` (only used if `WEB_AUTH_ENABLED` is `true`)
- `Config::OTA_PASSWORD`

`pio run` will fail with `fatal error: Config.h: No such file or directory` if this file is missing. This is also where GPIO pin assignments, pump schedules (`POOL_INTERVALS`, `PH_START`, `CL_START`), dose durations, dashboard language, and timing/watchdog constants are configured.

## Commands

| Command | Purpose |
|---|---|
| `pio run` | Build the firmware |
| `pio run -t upload` | Upload to a connected board |
| `pio device monitor` | Serial monitor (115200 baud) |
| `pio run -t clean` | Clean build artifacts |

There is no test suite and no linter configured — `pio run` is the only correctness check available.

## Dependencies

The four Arduino libraries are vendored under `lib/` instead of being declared via `lib_deps`, so the project builds fully offline (no PlatformIO registry access needed, e.g. on a school laptop without internet):

- `OneWire`
- `DallasTemperature`
- `esp32async/AsyncTCP` and `esp32async/ESPAsyncWebServer` (maintained forks — not the original `me-no-dev` packages)

Since they're vendored, they never update on their own. See **Scripts** below for how to check for and pull newer versions.

## Scripts

| Script | Purpose |
|---|---|
| `scripts/update-libs.sh` | Re-downloads the four libraries from the PlatformIO registry and replaces `lib/`. Run occasionally on a machine with internet access, then review `git diff lib/` and rebuild before committing. |
| `scripts/check_lib_updates.py` | Runs automatically as a PlatformIO pre-build hook (`extra_scripts` in `platformio.ini`). Non-blocking, rate-limited (once/24h) check that prints a note if newer library versions are available — never fails or slows down an offline build. |
| `scripts/list-boards.sh [filter]` | Lists PlatformIO boards for Espressif and Arduino platforms (`espressif32`, `espressif8266`, `atmelavr`, `atmelsam`), optionally filtered by ID/name substring. Useful for finding the right `board = <ID>` value for `platformio.ini`. |

## Architecture

- **`Config.h`** — single source of truth for WiFi credentials, web server port, NTP/timezone settings, GPIO pin assignments, pump schedules, dosing durations, WiFi reconnect behavior, main loop delay, and the temperature sensor's polling interval.
- **`PoolGuard.ino`** — entry point. `setup()` arms the task watchdog and starts `ArduinoOTA` once WiFi is up; `loop()` resets the watchdog, reconnects WiFi if dropped, re-syncs time if needed, services OTA, and calls `ServerManager::handleLogic()`.
- **`ServerManager`** — the core controller. Holds three `Device` structs (pool, pH, chlorine) each with a pin, mode, and time windows; `handleLogic()` evaluates schedules and drives GPIO pins with the pump safety interlock; owns the `AsyncWebServer` and DS18B20 sensor; registers `GET /`, `POST /set?dev=<pool|ph|cl>&m=<0|1|2>`, and `GET /status`.
- **`WifiManager`** — STA-mode connection setup/retry.
- **`PoolTime` (`DailyTimeSync`)** — NTP time sync, re-synced periodically.
- **`DS18B20`** — non-blocking OneWire/DallasTemperature wrapper; `update()` must be polled regularly to refresh readings.

See `CLAUDE.md` for more implementation detail.
