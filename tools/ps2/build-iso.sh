#!/bin/sh

set -eu

usage() {
    cat <<'EOF'
Usage: tools/ps2/build-iso.sh [options]

Options:
  --data-dir PATH    Include an owned ShadowFlare data directory in a
                     personal-use ISO. Omit for a data-free distributable ISO.
  --out-dir PATH     Directory for the ISO and disc files.
                     Default: build/ps2
  -h, --help         Show this help text.
EOF
}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd -P)
data_dir=
out_dir="$repo_root/build/ps2"
PS2DEV=${PS2DEV:-/usr/local/ps2dev}
PS2SDK=${PS2SDK:-$PS2DEV/ps2sdk}
GSKIT=${GSKIT:-$PS2DEV/gsKit}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --data-dir)
            [ "$#" -ge 2 ] || { echo "--data-dir needs a path" >&2; exit 2; }
            data_dir=$2
            shift 2
            ;;
        --out-dir)
            [ "$#" -ge 2 ] || { echo "--out-dir needs a path" >&2; exit 2; }
            out_dir=$2
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [ -n "$data_dir" ]; then
    if [ ! -d "$data_dir" ]; then
        echo "Data directory not found: $data_dir" >&2
        exit 1
    fi
    data_dir=$(CDPATH= cd -- "$data_dir" && pwd -P)
fi

mkdir -p "$out_dir"
out_dir=$(CDPATH= cd -- "$out_dir" && pwd -P)

if [ -n "$data_dir" ]; then
    missing_paths=
    for required_path in SFlare.Cfg System Scenario Save Player Map Character; do
        if [ ! -e "$data_dir/$required_path" ]; then
            missing_paths="$missing_paths $required_path"
        fi
    done
    if [ -n "$missing_paths" ]; then
        echo "Data directory '$data_dir' is missing:$missing_paths" >&2
        exit 1
    fi
fi

if [ ! -f "$PS2SDK/ps2dev.cmake" ]; then
    echo "PS2SDK CMake toolchain not found: $PS2SDK/ps2dev.cmake" >&2
    echo "Set PS2DEV/PS2SDK after installing PS2DEV." >&2
    exit 1
fi
if ! command -v cmake >/dev/null 2>&1 ||
   ! command -v genisoimage >/dev/null 2>&1; then
    echo "cmake and genisoimage are required to build the PS2 ISO." >&2
    exit 1
fi
export PS2DEV PS2SDK GSKIT

echo "== build ISO into $out_dir =="
if [ -n "$data_dir" ]; then
    REPO="$repo_root" DATA="$data_dir" OUT="$out_dir" \
        INCLUDE_GAME_DATA=1 sh "$script_dir/make-iso.sh"
    echo "Personal-use ISO ready: $out_dir/openshadowflare.iso"
else
    REPO="$repo_root" OUT="$out_dir" sh "$script_dir/make-iso.sh"
    echo "Data-free ISO ready: $out_dir/openshadowflare.iso"
    echo "Use --data-dir <owned-game-data> to build a personal-use disc."
fi
