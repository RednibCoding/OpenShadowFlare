# PS2 port

## Build

Install PS2DEV and `genisoimage`, then set its environment variables:

```sh
export PS2DEV="$HOME/ps2dev"
export PS2SDK="$PS2DEV/ps2sdk"
export GSKIT="$PS2DEV/gsKit"
export PATH="$PATH:$PS2DEV/bin:$PS2DEV/ee/bin:$PS2DEV/iop/bin"
```

On Ubuntu or WSL, install the PS2DEV prebuilt toolchain and build:

```sh
sudo apt-get update && sudo apt-get install -y curl cmake genisoimage
mkdir -p "$PS2DEV"
curl -fL https://github.com/ps2dev/ps2dev/releases/download/latest/ps2dev-ubuntu-latest.tar.gz \
  | tar -xz --strip-components=1 -C "$PS2DEV"
sh tools/ps2/build-iso.sh
```

The archive is published on [PS2DEV releases](https://github.com/ps2dev/ps2dev/releases).

The default output, `build/ps2/openshadowflare.iso`, is a small data-free ISO
suitable for CI and distribution. GitHub Actions builds and uploads this
variant, and verifies it does not contain game data. It loads an owned
`ShadowFlare` data directory placed beside the ISO in PCSX2, or at the root of
a FAT32 USB drive on PS2 hardware:

```text
<PCSX2 ISO folder>/
  openshadowflare.iso
  ShadowFlare/
    SFlare.Cfg
    System/
    Scenario/
    Save/
    Player/
    Map/
    Character/

FAT32 USB drive/
  ShadowFlare/
    SFlare.Cfg
    ...
```

In PCSX2, enable **Settings > Emulation > Enable Host Filesystem**. PCSX2
then maps its ISO folder to the `host:` device. The PS2 ISO includes the USB
drivers required to use `mass:` on hardware. Copy the owned game data:

```sh
# PCSX2: copy next to build/ps2/openshadowflare.iso.
cp -a /path/to/ShadowFlare build/ps2/

# PS2 hardware: copy to the root of a mounted FAT32 USB drive.
cp -a /path/to/ShadowFlare /media/$USER/PS2USB/
```

To create a private all-in-one disc from an owned retail installation, pass
the path to its `ShadowFlare` directory. This replaces the same output ISO with
a version containing a packed `SFGAME.BIN` archive, so it must not be
distributed:

```sh
sh tools/ps2/build-iso.sh --data-dir /path/to/ShadowFlare

# Example when the data is on the Windows E: drive in WSL.
sh tools/ps2/build-iso.sh --data-dir /mnt/e/Games/pcsx2/ps2/ShadowFlare
```

## Type portability

On PS2DEV, `std::int32_t` is `long`, not `int`. Use typed literals such as
`std::int32_t{0}` with `std::min`, `std::max`, and `std::clamp`; their
arguments must have the same type. Format fixed-width integers with `PRId32`
from `<cinttypes>` rather than `%d`.
