#!/usr/bin/env bash
# Re-vendors the Arduino libraries under lib/ from the PlatformIO registry.
#
# Libraries live in lib/ (not lib_deps in platformio.ini) so the project
# builds fully offline, e.g. on a school laptop without internet access.
# That means they never update on their own. Run this script occasionally
# on a machine WITH internet access to pull fresh versions, then review
# `git diff lib/` and rebuild with `pio run` before committing.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LIB_DIR="$REPO_ROOT/lib"
WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

# Bump version constraints here to pull newer releases.
LIBS=(
  "paulstoffregen/OneWire@^2.3.7"
  "milesburton/DallasTemperature@^3.11.0"
  "esp32async/AsyncTCP@^3.5.0"
  "esp32async/ESPAsyncWebServer@^3.12.0"
)

{
  echo "[env:esp32dev]"
  echo "platform = espressif32"
  echo "board = esp32dev"
  echo "framework = arduino"
  echo "lib_deps ="
  printf '    %s\n' "${LIBS[@]}"
} > "$WORKDIR/platformio.ini"

mkdir -p "$WORKDIR/src"
echo "void setup() {} void loop() {}" > "$WORKDIR/src/main.cpp"

echo "Downloading libraries via PlatformIO..."
( cd "$WORKDIR" && pio pkg install )

for dir in "$WORKDIR"/.pio/libdeps/esp32dev/*/; do
  name="$(basename "$dir")"
  echo "Vendoring $name..."
  rm -rf "${LIB_DIR:?}/$name"
  cp -r "$dir" "$LIB_DIR/$name"
  rm -rf "$LIB_DIR/$name/.git" "$LIB_DIR/$name/.github" "$LIB_DIR/$name/examples" "$LIB_DIR/$name/test"
done

# ESPAsyncWebServer's library.json declares dependencies for platforms we
# don't target (RPAsyncTCP for RP2040, ESPAsyncTCP for ESP8266). Without
# trimming this, PlatformIO's dependency finder tries to fetch them from
# the network during `pio run`, even though we only build for espressif32.
esp_async_ws_json="$LIB_DIR/ESPAsyncWebServer/library.json"
if [ -f "$esp_async_ws_json" ]; then
  echo "Trimming ESPAsyncWebServer/library.json to espressif32 only..."
  jq '.platforms = ["espressif32"]
      | .dependencies = [.dependencies[] | select(.name == "AsyncTCP") | .platforms = ["espressif32"]]' \
    "$esp_async_ws_json" > "$esp_async_ws_json.tmp"
  mv "$esp_async_ws_json.tmp" "$esp_async_ws_json"
fi

echo
echo "Done. Review and verify before committing:"
echo "  git status lib/"
echo "  git diff lib/"
echo "  pio run"
