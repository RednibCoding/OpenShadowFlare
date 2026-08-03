# Builds the PlayStation 2 disc image for OpenShadowFlare.
#
# Invoked by build-iso.sh on a host with PS2DEV installed. Produces a data-free
# ISO by default; INCLUDE_GAME_DATA=1 makes a local personal-use disc with a
# packed SFGAME.BIN archive.

set -u
: "${PS2DEV:=/usr/local/ps2dev}"
: "${PS2SDK:=$PS2DEV/ps2sdk}"
: "${GSKIT:=$PS2DEV/gsKit}"
export PS2DEV PS2SDK GSKIT
export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PS2SDK/ee/bin:$PS2SDK/iop/bin

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
REPO=${REPO:-$(CDPATH= cd -- "$script_dir/../.." && pwd -P)}
DATA=${DATA:-}
OUT=${OUT:-$REPO/build/ps2}
ELF_NAME=${ELF_NAME:-OPENSHAD.ELF}
INCLUDE_GAME_DATA=${INCLUDE_GAME_DATA:-0}
DISC_ROOT=$(mktemp -d /tmp/openshadowflare-disc.XXXXXX)
trap 'rm -rf "$DISC_ROOT"' EXIT HUP INT TERM

mkdir -p "$OUT"

case "$INCLUDE_GAME_DATA" in
  0|1) ;;
  *) echo "INCLUDE_GAME_DATA must be 0 or 1" >&2; exit 2 ;;
esac

if [ "$INCLUDE_GAME_DATA" = 1 ]; then
  if [ -z "$DATA" ] || [ ! -d "$DATA" ]; then
    echo "Game data is required when INCLUDE_GAME_DATA=1" >&2
    exit 1
  fi
  echo "== pack game data for personal-use disc =="
  gcc -O2 -Wall -o /tmp/pack "$REPO/tools/ps2/pack.c" || exit 1
  /tmp/pack "$DATA" "$DISC_ROOT/SFGAME.BIN" || exit 1
fi

echo "== build game =="
rm -rf /tmp/ps2build
cmake -S "$REPO" -B /tmp/ps2build \
  -DCMAKE_TOOLCHAIN_FILE=$PS2SDK/ps2dev.cmake \
  -DBUILD_TESTING=OFF \
  -DOPENSHADOWFLARE_BUILD_EXE=ON || exit 1
cmake --build /tmp/ps2build -j4 -- -k || exit 1
ELF=$(find /tmp/ps2build -name 'ShadowFlare_rebuilt*' -type f | head -1)
if [ -z "$ELF" ]; then
  echo "make-iso: no ELF produced" >&2
  exit 1
fi
cp "$ELF" "$DISC_ROOT/$ELF_NAME"

echo "== copy IOP modules =="
cp "$PS2SDK/iop/irx/iomanX.irx" "$DISC_ROOT/IOMANX.IRX" || exit 1
cp "$PS2SDK/iop/irx/fileXio.irx" "$DISC_ROOT/FILEXIO.IRX" || exit 1
cp "$PS2SDK/iop/irx/sio2man.irx" "$DISC_ROOT/SIO2MAN.IRX" || exit 1
cp "$PS2SDK/iop/irx/padman.irx" "$DISC_ROOT/PADMAN.IRX" || exit 1
cp "$PS2SDK/iop/irx/audsrv.irx" "$DISC_ROOT/AUDSRV.IRX" || exit 1
cp "$PS2SDK/iop/irx/usbd.irx" "$DISC_ROOT/USBD.IRX" || exit 1
cp "$PS2SDK/iop/irx/usbhdfsd.irx" "$DISC_ROOT/USBHDFSD.IRX" || exit 1

echo "== build ISO =="
printf 'BOOT2 = cdrom0:\\%s;1\nVER = 1.01\nVMODE = NTSC\n' "$ELF_NAME" > "$DISC_ROOT/SYSTEM.CNF"
rm -f "$OUT/openshadowflare.iso"
set -- \
  "$DISC_ROOT/SYSTEM.CNF" \
  "$DISC_ROOT/$ELF_NAME" \
  "$DISC_ROOT/IOMANX.IRX" \
  "$DISC_ROOT/FILEXIO.IRX" \
  "$DISC_ROOT/SIO2MAN.IRX" \
  "$DISC_ROOT/PADMAN.IRX" \
  "$DISC_ROOT/AUDSRV.IRX" \
  "$DISC_ROOT/USBD.IRX" \
  "$DISC_ROOT/USBHDFSD.IRX"
if [ "$INCLUDE_GAME_DATA" = 1 ]; then
  set -- "$@" "$DISC_ROOT/SFGAME.BIN"
fi
genisoimage -iso-level 2 -R -J -V OPENSHDOW -o "$OUT/openshadowflare.iso" "$@" \
  || exit 1
ls -la "$OUT/openshadowflare.iso"
