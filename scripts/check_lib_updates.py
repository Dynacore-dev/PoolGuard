"""PlatformIO pre-build hook: warns if newer versions of the vendored
libraries in lib/ are available on the PlatformIO registry.

This is purely informational and never blocks the build. lib/ is vendored
on purpose so the project builds fully offline (e.g. on a school laptop
without internet access) - see scripts/update-libs.sh. If the registry
can't be reached within the timeout, the check is skipped silently.

The check itself is rate-limited to once per CHECK_INTERVAL_SECONDS so it
doesn't add network latency to every single build.
"""

Import("env")

import json
import os
import time
import urllib.error
import urllib.parse
import urllib.request

TIMEOUT_SECONDS = 2
# urlopen's timeout only bounds individual socket operations, not the total
# request duration - a peer trickling bytes with gaps just under that
# per-read timeout could otherwise stall the check indefinitely. This caps
# the whole read regardless of how the bytes arrive.
TOTAL_DEADLINE_SECONDS = 5
# Far larger than any real registry search response; guards against an
# unbounded/oversized body from a malicious or broken endpoint.
MAX_RESPONSE_BYTES = 64 * 1024
CHECK_INTERVAL_SECONDS = 24 * 60 * 60
REGISTRY_SEARCH_URL = "https://api.registry.platformio.org/v3/search"

# (directory name under lib/, registry owner, registry name)
LIBRARIES = [
    ("OneWire", "paulstoffregen", "OneWire"),
    ("DallasTemperature", "milesburton", "DallasTemperature"),
    ("AsyncTCP", "esp32async", "AsyncTCP"),
    ("ESPAsyncWebServer", "esp32async", "ESPAsyncWebServer"),
]


def local_version(lib_dir):
    path = os.path.join(env["PROJECT_DIR"], "lib", lib_dir, "library.json")
    try:
        with open(path) as f:
            return json.load(f).get("version")
    except (OSError, ValueError):
        return None


def _read_with_deadline(resp, max_bytes, deadline_seconds):
    deadline = time.time() + deadline_seconds
    chunks = []
    total = 0
    while True:
        if time.time() > deadline:
            raise TimeoutError("registry response exceeded the total time budget")
        chunk = resp.read(8192)
        if not chunk:
            break
        total += len(chunk)
        if total > max_bytes:
            raise ValueError("registry response exceeded the maximum allowed size")
        chunks.append(chunk)
    return b"".join(chunks)


def latest_registry_version(owner, name):
    query = urllib.parse.urlencode({"query": f"owner:{owner} name:{name}"})
    with urllib.request.urlopen(f"{REGISTRY_SEARCH_URL}?{query}", timeout=TIMEOUT_SECONDS) as resp:
        body = _read_with_deadline(resp, MAX_RESPONSE_BYTES, TOTAL_DEADLINE_SECONDS)
    data = json.loads(body)
    items = data.get("items") or []
    if not items:
        return None
    return items[0].get("version", {}).get("name")


def cache_path():
    return os.path.join(env["PROJECT_DIR"], ".pio", "lib_update_check.json")


def should_skip_check():
    try:
        with open(cache_path()) as f:
            last_checked = json.load(f).get("last_checked", 0)
    except (OSError, ValueError):
        return False
    return (time.time() - last_checked) < CHECK_INTERVAL_SECONDS


def mark_checked():
    path = cache_path()
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w") as f:
        json.dump({"last_checked": time.time()}, f)


def main():
    if should_skip_check():
        return

    outdated = []
    for lib_dir, owner, name in LIBRARIES:
        current = local_version(lib_dir)
        if not current:
            continue
        try:
            latest = latest_registry_version(owner, name)
        except (OSError, ValueError, urllib.error.URLError, KeyError, TypeError, IndexError, AttributeError):
            # No internet, registry unreachable, or an unexpected response
            # shape - stay silent, don't block or slow down the build.
            return
        if latest and latest != current:
            outdated.append((lib_dir, current, latest))

    mark_checked()

    if outdated:
        print()
        print("[check_lib_updates] Neuere Versionen verfuegbar (lib/ ist vendored, kein Auto-Update):")
        for lib_dir, current, latest in outdated:
            print(f"[check_lib_updates]   {lib_dir}: {current} -> {latest}")
        print("[check_lib_updates]   Zum Aktualisieren: ./scripts/update-libs.sh")
        print()


main()
