#!/bin/bash

source ../tools/emsdk/emsdk_env.sh

BUILD_DIR="../build-wasm"

if [ "$1" = "--fresh" ] || [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    emcmake cmake -S port -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
fi

cmake --build "$BUILD_DIR"
