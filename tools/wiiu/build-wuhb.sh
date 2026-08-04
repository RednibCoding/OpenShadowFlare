#!/usr/bin/env sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
WIIU_CMAKE="$DEVKITPRO/portlibs/wiiu/bin/powerpc-eabi-cmake"
BUILD_DIR="$ROOT_DIR/build/wiiu/release"
OUTPUT_WUHB="$BUILD_DIR/OpenShadowFlare.wuhb"

if [ ! -x "$WIIU_CMAKE" ]; then
    echo "The devkitPro Wii U CMake wrapper was not found: $WIIU_CMAKE" >&2
    echo "Install wiiu-dev and set DEVKITPRO if it is not /opt/devkitpro." >&2
    exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "Ninja is required to build the Wii U port." >&2
    exit 1
fi

export DEVKITPRO

"$WIIU_CMAKE" -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DOPENSHADOWFLARE_BUILD_EXE=OFF
cmake --build "$BUILD_DIR" --parallel

WUHB=$(find "$BUILD_DIR" -type f -name '*.wuhb' ! -path "$OUTPUT_WUHB" -print -quit)
if [ -z "$WUHB" ]; then
    echo "CMake did not produce a WUHB under: $BUILD_DIR" >&2
    exit 1
fi

if [ "$WUHB" != "$OUTPUT_WUHB" ]; then
    cp "$WUHB" "$OUTPUT_WUHB"
fi
echo "WUHB: $OUTPUT_WUHB"
