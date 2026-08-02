#!/usr/bin/env sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake"
BUILD_DIR="$ROOT_DIR/build/switch/release"
NRO="$BUILD_DIR/src/SF_EXE/ShadowFlare_rebuilt.nro"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "Nintendo Switch devkitPro tools were not found under: $DEVKITPRO" >&2
    exit 1
fi

export DEVKITPRO

if ! command -v ninja >/dev/null 2>&1; then
    echo "Ninja is required to build the Switch port." >&2
    exit 1
fi

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF
cmake --build "$BUILD_DIR" --parallel

if [ ! -s "$NRO" ]; then
    echo "CMake did not produce the expected NRO: $NRO" >&2
    exit 1
fi

echo "NRO: $NRO"
