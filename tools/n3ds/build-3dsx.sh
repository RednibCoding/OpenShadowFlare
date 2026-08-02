#!/usr/bin/env sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
TOOLCHAIN_FILE="$DEVKITPRO/cmake/3DS.cmake"
BUILD_DIR="$ROOT_DIR/build/n3ds/release"
THREEDSX="$BUILD_DIR/src/SF_EXE/OpenShadowFlare.3dsx"

if [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "Nintendo 3DS devkitPro tools were not found under: $DEVKITPRO" >&2
    exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "Ninja is required to build the Nintendo 3DS port." >&2
    exit 1
fi

export DEVKITPRO

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF
cmake --build "$BUILD_DIR" --parallel

if [ ! -s "$THREEDSX" ]; then
    echo "CMake did not produce the expected 3DSX: $THREEDSX" >&2
    exit 1
fi

echo "3DSX: $THREEDSX"
