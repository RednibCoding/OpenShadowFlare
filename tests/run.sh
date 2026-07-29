#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

BUILD_CONFIG=release
RUN_WINE=false
while [ "$#" -gt 0 ]; do
    case "$1" in
        --config)
            if [ "$#" -lt 2 ]; then
                echo "Error: --config needs debug or release." >&2
                exit 1
            fi
            BUILD_CONFIG="$2"
            shift
            ;;
        --wine)
            RUN_WINE=true
            ;;
        *)
            echo "Error: unknown argument: $1" >&2
            exit 1
            ;;
    esac
    shift
done

case "$BUILD_CONFIG" in
    debug|release) ;;
    *)
        echo "Error: configuration must be debug or release." >&2
        exit 1
        ;;
esac

"$ROOT_DIR/src/build.sh" --config "$BUILD_CONFIG"
OSF_BUILD_CONFIG="$BUILD_CONFIG" \
    python3 "$ROOT_DIR/tools/verify_fidelity.py"
python3 "$ROOT_DIR/tools/pe_imports.py" \
    "$ROOT_DIR/tmp/ShadowFlare/ShadowFlare.exe" --summary >/dev/null

if [ "$RUN_WINE" = true ]; then
    OSF_BUILD_CONFIG="$BUILD_CONFIG" \
        "$ROOT_DIR/tests/differential/run_foundation.sh"
    OSF_BUILD_CONFIG="$BUILD_CONFIG" \
        "$ROOT_DIR/tests/integration/smoke_wine.sh"
else
    echo "Static tests passed. Add --wine to run behavioral differential probes."
fi
