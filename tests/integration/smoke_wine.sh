#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
ORIGINAL_DIR="$ROOT_DIR/tmp/ShadowFlare"
REBUILT_DIR="$ROOT_DIR/src/build-win32"
DURATION="${OSF_SMOKE_SECONDS:-20}"

if ! command -v wine >/dev/null 2>&1; then
    echo "Error: wine is required." >&2
    exit 1
fi
if [ ! -f "$REBUILT_DIR/RKC_UPDIB.dll" ]; then
    echo "Error: rebuilt DLLs are missing; run ./src/build.sh first." >&2
    exit 1
fi

TEST_ROOT="$(mktemp -d /tmp/openshadowflare-smoke.XXXXXX)"
cleanup() {
    if [ -d "$TEST_ROOT" ]; then
        find "$TEST_ROOT" -depth -delete
    fi
}
trap cleanup EXIT

cp -a --reflink=auto "$ORIGINAL_DIR" "$TEST_ROOT/game"
mkdir "$TEST_ROOT/wineprefix"

for rebuilt_dll in "$REBUILT_DIR"/*.dll; do
    dll_name="$(basename "$rebuilt_dll")"
    if [ ! -f "$TEST_ROOT/game/o_$dll_name" ]; then
        mv "$TEST_ROOT/game/$dll_name" "$TEST_ROOT/game/o_$dll_name"
    fi
done
cp "$REBUILT_DIR"/*.dll "$TEST_ROOT/game/"

set +e
(
    cd "$TEST_ROOT/game"
    WINEPREFIX="$TEST_ROOT/wineprefix" \
    WINEARCH=win32 \
    WINEDEBUG=-all \
    OSF_TRACE=0 \
    OSF_DBF_LOG=1 \
    timeout --signal=TERM --kill-after=5s "${DURATION}s" wine ShadowFlare.exe \
        >"$TEST_ROOT/wine.stdout" \
        2>"$TEST_ROOT/wine.stderr"
)
status=$?
set -e

if [ "$status" -ne 0 ] && [ "$status" -ne 124 ]; then
    echo "Wine smoke test exited unexpectedly with status $status." >&2
    sed -n '1,200p' "$TEST_ROOT/wine.stderr" >&2
    exit 1
fi

DBF_LOG="$TEST_ROOT/game/dbfcontrol_log.txt"
if [ ! -f "$DBF_LOG" ]; then
    echo "Smoke test did not initialize the reconstructed presentation DLL." >&2
    exit 1
fi
if ! grep -q "OpenGL initialized:" "$DBF_LOG"; then
    echo "Smoke test did not reach OpenGL initialization." >&2
    exit 1
fi
if ! grep -q "Paint: mode=" "$DBF_LOG"; then
    echo "Smoke test did not reach the render loop." >&2
    exit 1
fi
if find "$TEST_ROOT/game" -maxdepth 1 -type f -iname '*crash*.log' | grep -q .; then
    echo "Smoke test generated a crash log." >&2
    exit 1
fi
if grep -Eiq 'error|failed|exception|crash' "$DBF_LOG"; then
    echo "Smoke test presentation log contains an error." >&2
    grep -Ein 'error|failed|exception|crash' "$DBF_LOG" >&2
    exit 1
fi

paint_count="$(grep -c "Paint: mode=" "$DBF_LOG")"
echo "PASS: Wine smoke test reached the render loop ($paint_count paint calls in ${DURATION}s)."
