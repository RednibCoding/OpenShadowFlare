# Builds the PlayStation 2 disc image for OpenShadowFlare.
#
# Runs inside the openshadowflare-ps2 Docker image:
#   docker run --rm -it \
#     -v <repo>:/mnt/repo \
#     -v <game-data>:/hostdata \
#     -v <out-dir>:/out \
#     openshadowflare-ps2 sh /mnt/repo/tools/ps2/make-iso.sh
#
# Produces /out/openshadowflare.iso: SYSTEM.CNF + boot ELF + SFGAME.BIN (the
# packed game-data archive read through ps2_data_backend.cpp).  The data tree
# itself is intentionally NOT on the disc: the BIOS fileio module strips path
# separators and matches ISO9660 names case-sensitively, so it can only reach
# flat uppercase root files anyway.

set -u
export PS2SDK=/usr/local/ps2dev/ps2sdk
export PS2DEV=/usr/local/ps2dev
export GSKIT=$PS2DEV/gsKit
export PATH=$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin:$PS2SDK/bin:$PS2SDK/ee/bin:$PS2SDK/iop/bin

REPO=${REPO:-/mnt/repo}
DATA=${DATA:-/hostdata}
OUT=${OUT:-/out}
ELF_NAME=${ELF_NAME:-OPENSHAD.ELF}

echo "== pack game data =="
gcc -O2 -Wall -o /tmp/pack "$REPO/tools/ps2/pack.c" || exit 1
/tmp/pack "$DATA" "$OUT/SFGAME.BIN" || exit 1

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
cp "$ELF" "$OUT/$ELF_NAME"

echo "== copy IOP modules =="
cp "$PS2SDK/iop/irx/iomanX.irx" "$OUT/IOMANX.IRX" || exit 1
cp "$PS2SDK/iop/irx/fileXio.irx" "$OUT/FILEXIO.IRX" || exit 1
cp "$PS2SDK/iop/irx/sio2man.irx" "$OUT/SIO2MAN.IRX" || exit 1
cp "$PS2SDK/iop/irx/padman.irx" "$OUT/PADMAN.IRX" || exit 1
cp "$PS2SDK/iop/irx/audsrv.irx" "$OUT/AUDSRV.IRX" || exit 1

echo "== build ISO =="
printf 'BOOT2 = cdrom0:\\%s;1\nVER = 1.01\nVMODE = NTSC\n' "$ELF_NAME" > "$OUT/SYSTEM.CNF"
rm -f "$OUT/openshadowflare.iso"
genisoimage -iso-level 2 -R -J -V OPENSHDOW -o "$OUT/openshadowflare.iso" \
  "$OUT/SYSTEM.CNF" \
  "$OUT/$ELF_NAME" \
  "$OUT/SFGAME.BIN" \
  "$OUT/IOMANX.IRX" \
  "$OUT/FILEXIO.IRX" \
  "$OUT/SIO2MAN.IRX" \
  "$OUT/PADMAN.IRX" \
  "$OUT/AUDSRV.IRX" \
  || exit 1
ls -la "$OUT/openshadowflare.iso"
