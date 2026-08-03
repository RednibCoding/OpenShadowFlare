#!/usr/bin/env sh
set -eu

if [ -z "${PSPDEV:-}" ]; then
  echo "PSPDEV is not set. Install PSPDEV and export PSPDEV=/path/to/pspdev." >&2
  exit 1
fi

build_dir="build/psp/release"
output="$build_dir/OpenShadowFlare-psp-release.PBP"

cmake -S . -B "$build_dir" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PSPDEV/psp/share/pspdev.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --parallel

pbp="$(find "$build_dir" -type f -name EBOOT.PBP -print -quit)"
if [ -z "$pbp" ]; then
  echo "The PSP build completed without producing EBOOT.PBP." >&2
  exit 1
fi

cp "$pbp" "$output"
printf 'PBP: %s\n' "$output"
