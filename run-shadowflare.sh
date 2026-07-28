#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
GAME_DIR="$ROOT_DIR/tmp/ShadowFlare"
GAME_EXE="$GAME_DIR/ShadowFlare.exe"

if ! command -v wine >/dev/null 2>&1; then
    echo "Error: wine is not installed or not in PATH." >&2
    exit 1
fi

if [ ! -f "$GAME_EXE" ]; then
    echo "Error: game executable not found at $GAME_EXE" >&2
    exit 1
fi

DEFAULT_WINE_PREFIX="${XDG_DATA_HOME:-${HOME}/.local/share}/openshadowflare/wineprefix"
export WINEPREFIX="${WINEPREFIX:-$DEFAULT_WINE_PREFIX}"
export WINEARCH="${WINEARCH:-win32}"
mkdir -p "$WINEPREFIX"

cd "$GAME_DIR"
exec wine "$GAME_EXE" "$@"
