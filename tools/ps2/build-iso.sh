#!/bin/sh

set -eu

usage() {
    cat <<'EOF'
Usage: tools/ps2/build-iso.sh [options]

Options:
  --data-dir PATH    Original ShadowFlare data directory.
                     Default: tmp/ShadowFlare
  --out-dir PATH     Directory for the ISO and disc files.
                     Default: build/ps2
  --image NAME       Docker image name. Default: openshadowflare-ps2
  --build-image      Rebuild the Docker image before packaging.
  -h, --help         Show this help text.
EOF
}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd -P)
data_dir="$repo_root/tmp/ShadowFlare"
out_dir="$repo_root/build/ps2"
image=openshadowflare-ps2
build_image=0

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
        --image)
            [ "$#" -ge 2 ] || { echo "--image needs a name" >&2; exit 2; }
            image=$2
            shift 2
            ;;
        --build-image)
            build_image=1
            shift
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

if [ ! -d "$data_dir" ]; then
    echo "Data directory not found: $data_dir" >&2
    exit 1
fi
data_dir=$(CDPATH= cd -- "$data_dir" && pwd -P)

mkdir -p "$out_dir"
out_dir=$(CDPATH= cd -- "$out_dir" && pwd -P)

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

docker_run() {
    case "$(uname -s)" in
        MINGW*|MSYS*)
            MSYS_NO_PATHCONV=1 docker "$@"
            ;;
        *)
            docker "$@"
            ;;
    esac
}

docker_host_path() {
    case "$(uname -s)" in
        MINGW*|MSYS*)
            cygpath -w "$1"
            ;;
        *)
            printf '%s\n' "$1"
            ;;
    esac
}

repo_mount=$(docker_host_path "$repo_root")
data_mount=$(docker_host_path "$data_dir")
out_mount=$(docker_host_path "$out_dir")

if [ "$build_image" -eq 0 ] && ! docker image inspect "$image" >/dev/null 2>&1; then
    build_image=1
fi
if [ "$build_image" -eq 1 ]; then
    echo "== build Docker image =="
    docker_run build -t "$image" -f "$repo_mount/tools/ps2/Dockerfile" "$repo_mount"
fi

echo "== build ISO into $out_dir =="
docker_run run --rm \
    -v "$repo_mount:/mnt/repo" \
    -v "$data_mount:/hostdata:ro" \
    -v "$out_mount:/out" \
    "$image" \
    sh /mnt/repo/tools/ps2/make-iso.sh

echo "ISO ready: $out_dir/openshadowflare.iso"
