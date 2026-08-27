#!/usr/bin/env bash
# Listet PlatformIO-Boards fuer Espressif- und Arduino-Plattformen auf.
# Optional: ein Suchbegriff als Argument filtert nach ID/Name (Substring,
# Gross-/Kleinschreibung egal).
#
# Beispiele:
#   ./scripts/list-boards.sh              # komplette Liste aller Plattformen
#   ./scripts/list-boards.sh esp32-s3     # nur passende ESP32-S3-Boards
#   ./scripts/list-boards.sh mega         # z.B. Arduino Mega
#
# Gefundene ID einfach als "board = <ID>" in platformio.ini eintragen.
set -euo pipefail

PLATFORMS=(espressif32 espressif8266 atmelavr atmelsam)
FILTER="${1:-}"

for platform in "${PLATFORMS[@]}"; do
  raw="$(pio boards "$platform")"

  if [ -z "$FILTER" ]; then
    echo "$raw"
    echo
    continue
  fi

  filtered="$(echo "$raw" | awk -v f="$FILTER" '
    BEGIN { f = tolower(f) }
    NR <= 4 { header = header $0 "\n"; next }
    tolower($0) ~ f { body = body $0 "\n" }
    END { if (body != "") printf "%s%s", header, body }
  ')"

  if [ -n "$filtered" ]; then
    echo "$filtered"
    echo
  fi
done
