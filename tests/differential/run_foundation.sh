#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
PROBE_SOURCE="$ROOT_DIR/tests/differential/foundation_probe.cpp"
PROBE_EXE="$ROOT_DIR/tests/differential/foundation_probe.exe"
ORIGINAL_DIR="$ROOT_DIR/tmp/ShadowFlare"
REBUILT_DIR="$ROOT_DIR/src/build-win32"
ORIGINAL_PREFIX=""
if [ -f "$ORIGINAL_DIR/o_RKC_FILE.dll" ]; then
    ORIGINAL_PREFIX="o_"
fi

if ! command -v i686-w64-mingw32-g++ >/dev/null 2>&1; then
    echo "Error: i686-w64-mingw32-g++ is required." >&2
    exit 1
fi
if ! command -v wine >/dev/null 2>&1; then
    echo "Error: wine is required." >&2
    exit 1
fi

i686-w64-mingw32-g++ \
    -std=c++17 \
    -O2 \
    -Wall \
    -Wextra \
    -Wno-cast-function-type \
    -static-libgcc \
    -static-libstdc++ \
    -o "$PROBE_EXE" \
    "$PROBE_SOURCE" \
    -lgdi32

TEST_ROOT="$(mktemp -d /tmp/openshadowflare-differential.XXXXXX)"
cleanup() {
    if [ -d "$TEST_ROOT" ]; then
        find "$TEST_ROOT" -depth -delete
    fi
}
trap cleanup EXIT

mkdir "$TEST_ROOT/wineprefix"
export WINEPREFIX="$TEST_ROOT/wineprefix"
export WINEARCH=win32
export WINEDEBUG=-all
wineboot -u >/dev/null 2>&1

PROBE_WINDOWS="$(winepath -w "$PROBE_EXE")"
SCRATCH_WINDOWS="$(winepath -w "$TEST_ROOT/probe.bin")"
TABLE_WINDOWS="$(winepath -w "$ORIGINAL_DIR/System/Game/Parameter/Table.Tbd")"
LZ_SCRATCH_WINDOWS="$(winepath -w "$TEST_ROOT/lz.bin")"
AI_WINDOWS="$(winepath -w "$ORIGINAL_DIR/System/Game/Parameter/Control.aid")"
AI_SCRATCH_WINDOWS="$(winepath -w "$TEST_ROOT/control.aid")"
SCRIPT_WINDOWS="$(winepath -w "$ORIGINAL_DIR/Scenario/00000001/Scenario.Scs")"
SCRIPT_SCRATCH_WINDOWS="$(winepath -w "$TEST_ROOT/scenario.scs")"
FONT_SCRATCH_WINDOWS="$(winepath -w "$TEST_ROOT/font.njp")"

run_probe() {
    local label="$1"
    local original_dll="$2"
    local rebuilt_dll="$3"
    shift 3

    local original_windows
    local rebuilt_windows
    original_windows="$(winepath -w "$original_dll")"
    rebuilt_windows="$(winepath -w "$rebuilt_dll")"

    (
        cd "$(dirname "$original_dll")"
        wine "$PROBE_WINDOWS" "$original_windows" "$@"
    ) >"$TEST_ROOT/$label.original"
    (
        cd "$(dirname "$rebuilt_dll")"
        wine "$PROBE_WINDOWS" "$rebuilt_windows" "$@"
    ) >"$TEST_ROOT/$label.rebuilt"
    diff -u "$TEST_ROOT/$label.original" "$TEST_ROOT/$label.rebuilt"
    printf 'PASS: differential %-8s %s' "$label" "$(cat "$TEST_ROOT/$label.rebuilt")"
}

run_probe \
    file \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_FILE.dll" \
    "$REBUILT_DIR/RKC_FILE.dll" \
    file \
    "$SCRATCH_WINDOWS"
run_probe \
    memory \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_MEMORY.dll" \
    "$REBUILT_DIR/RKC_MEMORY.dll" \
    memory
run_probe \
    window \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_WINDOW.dll" \
    "$REBUILT_DIR/RKC_WINDOW.dll" \
    window
run_probe \
    dib \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_DIB.dll" \
    "$REBUILT_DIR/RKC_DIB.dll" \
    dib
run_probe \
    dib_hispeed \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_DIB.dll" \
    "$REBUILT_DIR/RKC_DIB.dll" \
    dib_hispeed
run_probe \
    table \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_RPG_TABLE.dll" \
    "$REBUILT_DIR/RKC_RPG_TABLE.dll" \
    table \
    "$TABLE_WINDOWS"
run_probe \
    updib \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_UPDIB.dll" \
    "$REBUILT_DIR/RKC_UPDIB.dll" \
    updib
run_probe \
    dbf \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_DBFCONTROL.dll" \
    "$REBUILT_DIR/RKC_DBFCONTROL.dll" \
    dbf
run_probe \
    rk_lz \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RK_FUNCTION.dll" \
    "$REBUILT_DIR/RK_FUNCTION.dll" \
    rk_lz \
    "$TABLE_WINDOWS" \
    "$LZ_SCRATCH_WINDOWS"
run_probe \
    rk_utils \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RK_FUNCTION.dll" \
    "$REBUILT_DIR/RK_FUNCTION.dll" \
    rk_utils
run_probe \
    font \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_FONTMAKER.dll" \
    "$REBUILT_DIR/RKC_FONTMAKER.dll" \
    font \
    "$FONT_SCRATCH_WINDOWS"
run_probe \
    ai \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_RPG_AICONTROL.dll" \
    "$REBUILT_DIR/RKC_RPG_AICONTROL.dll" \
    ai \
    "$AI_WINDOWS" \
    "$AI_SCRATCH_WINDOWS"
run_probe \
    script \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_RPG_SCRIPT.dll" \
    "$REBUILT_DIR/RKC_RPG_SCRIPT.dll" \
    script \
    "$SCRIPT_WINDOWS" \
    "$SCRIPT_SCRATCH_WINDOWS"
run_probe \
    network \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_NETWORK.dll" \
    "$REBUILT_DIR/RKC_NETWORK.dll" \
    network
run_probe \
    rpgscrn \
    "$ORIGINAL_DIR/${ORIGINAL_PREFIX}RKC_RPGSCRN.dll" \
    "$REBUILT_DIR/RKC_RPGSCRN.dll" \
    rpgscrn

echo "Foundation differential tests passed."
