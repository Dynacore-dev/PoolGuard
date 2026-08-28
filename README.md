# PoolGuard

PlatformIO/Arduino firmware for an ESP32 that controls a pool's filter pump plus two dosing pumps (pH, chlorine) on a schedule, exposes a small web dashboard, and reads water temperature from a DS18B20 sensor.

## Features

- **Scheduled pool pump** — up to 3 daily on/off time windows (`Config::POOL_INTERVALS`).
- **pH / chlorine dosing pumps** — start at a configured time and run for a configurable dose duration.
- **Safety interlock** — the pH/chlorine pumps only run in `AUTO`/`ON` mode while the pool pump's own output pin is driven HIGH. This is a GPIO read-back, not a real flow sensor or relay feedback signal, so it won't catch a relay/motor failure that leaves the pin driven HIGH.
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

## Installation

Step-by-step guide to go from a fresh machine to a flashed board. All commands below are PlatformIO Core CLI (`pio`) — the same commands work whether you run them from a plain terminal or from inside an IDE's built-in terminal (VS Code + PlatformIO IDE extension, or CLion + the PlatformIO plugin both wrap the same CLI).

### 1. Install prerequisites

You need Python 3.9+, Git, and (for USB uploads) a driver for the board's USB-to-UART chip — usually Silicon Labs **CP210x** or WCH **CH340**, printed on the chip next to the USB port.

#### Windows

1. Install [Python 3](https://www.python.org/downloads/windows/) — during setup, check **"Add python.exe to PATH"**.
2. Install [Git for Windows](https://git-scm.com/download/win).
3. Install the USB driver for your board's chip: [CP210x driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers) or [CH340 driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html), whichever matches your board. Reboot afterwards if prompted.
4. Open PowerShell and install PlatformIO via [pipx](https://pipx.pypa.io/) (keeps it isolated from other Python tools):
   ```powershell
   py -m pip install --user pipx
   py -m pipx ensurepath
   ```
   Close and reopen PowerShell, then:
   ```powershell
   pipx install platformio
   ```
5. Verify: `pio --version`.
6. When you plug in the board, it should appear as a `COMx` port in Device Manager under "Ports (COM & LPT)".

#### macOS

1. Install [Homebrew](https://brew.sh/) if you don't already have it.
2. Install Python and pipx:
   ```sh
   brew install python pipx
   pipx ensurepath
   ```
   Restart your terminal, then:
   ```sh
   pipx install platformio
   ```
3. Git is already included with Xcode Command Line Tools (`xcode-select --install` if `git` isn't found).
4. Install the [CP210x VCP driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers) if your board uses that chip (many newer macOS versions bundle CH340 support natively, but CP210x still needs the Silicon Labs driver). If macOS blocks the driver, allow it under **System Settings → Privacy & Security**.
5. Verify: `pio --version`. The board should show up as `/dev/tty.usbserial-*` or `/dev/tty.SLAB_USBtoUART`.

#### Linux (Debian/Ubuntu)

1. Install Python, pip, and Git:
   ```sh
   sudo apt update && sudo apt install -y python3 python3-pip python3-venv git
   ```
2. Install pipx and PlatformIO:
   ```sh
   python3 -m pip install --user pipx
   python3 -m pipx ensurepath
   ```
   Restart your terminal, then:
   ```sh
   pipx install platformio
   ```
3. Add your user to the `dialout` group so you can access `/dev/ttyUSB*` without `sudo`, then log out and back in (group changes need a new session):
   ```sh
   sudo usermod -aG dialout $USER
   ```
4. Verify: `pio --version`. The board should show up as `/dev/ttyUSB0` (or similar).
5. **Known conflict:** `ModemManager` (installed by default on many desktop distros) probes newly-plugged USB-serial devices and can corrupt the ESP32's serial output (garbled text in `pio device monitor`, or the port intermittently unusable) even though it never actually claims the device for a modem. If you hit this, tell it to permanently ignore this board's chip (adjust `idVendor`/`idProduct` if your board uses a different chip, e.g. CH340's `1a86`/`7523`):
   ```sh
   echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ENV{ID_MM_DEVICE_IGNORE}="1"' | sudo tee /etc/udev/rules.d/99-esp32-ignore-modemmanager.rules
   sudo udevadm control --reload-rules
   sudo udevadm trigger
   ```
   Unplug and replug the board afterwards.

### 2. Clone the repository

```sh
git clone https://github.com/Dynacore-dev/PoolGuard.git
cd PoolGuard
```

### 3. Configure secrets

Follow the **Setup** section below (`src/Config.h`) before building — `pio run` fails without it.

### 4. Build and flash via USB

Connect the ESP32 by USB, then from the project root:

```sh
pio run -t upload
```

PlatformIO auto-detects the port. If it picks the wrong one (e.g. multiple boards attached), pass it explicitly: `pio run -t upload --upload-port COM3` (Windows) or `--upload-port /dev/ttyUSB0` (macOS/Linux).

### 5. Watch the logs (optional)

```sh
pio device monitor
```

Close the monitor (Ctrl+C) before running another upload — the port can only be opened by one process at a time, and an open monitor will make `pio run -t upload` hang at "Connecting...".

### 6. Switch to OTA updates (optional)

Once the board has been flashed once via USB with `Config::OTA_ENABLED = true` and a real `Config::OTA_PASSWORD`, you can flash over WiFi instead of USB using the `esp32dev_ota` environment in `platformio.ini`. Set the same password as an environment variable (never commit it), then upload:

```sh
export OTA_PASSWORD=<same value as Config::OTA_PASSWORD>   # Windows PowerShell: $env:OTA_PASSWORD = "..."
pio run -e esp32dev_ota -t upload
```

This requires the board to already be powered on and connected to WiFi. `pio device monitor` does not work over OTA — it needs the USB serial connection.

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
| `pio run -t upload` | Upload to a connected board over USB |
| `pio run -e esp32dev_ota -t upload` | Upload over WiFi (OTA) — needs `OTA_PASSWORD` env var set; see Installation |
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
