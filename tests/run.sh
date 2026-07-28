#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"

"$ROOT_DIR/src/build.sh"
python3 "$ROOT_DIR/tools/verify_fidelity.py"
python3 "$ROOT_DIR/tools/pe_imports.py" \
    "$ROOT_DIR/tmp/ShadowFlare/ShadowFlare.exe" --summary >/dev/null

if [ "${1:-}" = "--wine" ]; then
    "$ROOT_DIR/tests/differential/run_foundation.sh"
    "$ROOT_DIR/tests/integration/smoke_wine.sh"
else
    echo "Static tests passed. Add --wine to run behavioral differential probes."
fi
