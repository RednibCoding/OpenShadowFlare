#!/usr/bin/env sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TOOLCHAIN_FILE="$DEVKITPRO/cmake/Switch.cmake"
BUILD_DIR="$ROOT_DIR/build/switch/release"
OUTPUT_NRO="$BUILD_DIR/OpenShadowFlare.nro"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "Nintendo Switch devkitPro tools were not found under: $DEVKITPRO" >&2
    exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "Ninja is required to build the Switch port." >&2
    exit 1
fi

export DEVKITPRO

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DOPENSHADOWFLARE_BUILD_EXE=OFF
cmake --build "$BUILD_DIR" --parallel

NRO=$(find "$BUILD_DIR" -type f -name '*.nro' ! -path "$OUTPUT_NRO" -print -quit)
if [ -z "$NRO" ]; then
    echo "CMake did not produce an NRO under: $BUILD_DIR" >&2
    exit 1
fi

if [ "$NRO" != "$OUTPUT_NRO" ]; then
    cp "$NRO" "$OUTPUT_NRO"
fi
echo "NRO: $OUTPUT_NRO"
