#!/bin/sh
# fm1-base's build on this repository's firmware (settings in deepfield/fm1.toml).
#   ./build.sh                 deepfield/build/deepfield.fwsc        (JieLi toolchain in a container, AC79 SDK)
#   ./build.sh --host          deepfield/build/host/deepfield_host   (the simulator; the system cc only)
#   ./build.sh --web           deepfield/build/emu/                  (the browser emulator; Emscripten)
#   ./build.sh --release X.Y   deepfield/build/deepfield-X.Y.fwsc    (deepfield/tools/publish_site.sh does this)
# A first argument naming another directory with an fm1.toml builds that one instead.
set -e
cd "$(dirname "$0")"
PYTHON="${PYTHON:-/opt/homebrew/bin/python3.14}"
APP=deepfield
if [ -n "$1" ] && [ -f "$1/fm1.toml" ]; then
    APP="$1"
    shift
fi
exec "$PYTHON" base/tools/build.py "$APP" "$@"
