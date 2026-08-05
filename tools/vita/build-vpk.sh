#!/usr/bin/env sh

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
VITASDK=${VITASDK:-/usr/local/vitasdk}
BUILD_DIR="$ROOT_DIR/build/vita/release"
VPK="$BUILD_DIR/shadowflare/OpenShadowFlare.vpk"

if [ -z "$VITASDK" ] || [ ! -f "$VITASDK/share/vita.toolchain.cmake" ]; then
    echo "VitaSDK was not found. Set VITASDK to its installation directory." >&2
    exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "Ninja is required to build the Vita port." >&2
    exit 1
fi

export VITASDK

cmake -S "$ROOT_DIR" -B "$BUILD_DIR" \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DOPENSHADOWFLARE_BUILD_EXE=OFF \
    -DOPENSHADOWFLARE_C99_PRESENTATION_HZ=30
cmake --build "$BUILD_DIR" --parallel

if [ ! -s "$VPK" ]; then
    echo "CMake did not produce the expected VPK: $VPK" >&2
    exit 1
fi

echo "VPK: $VPK"
