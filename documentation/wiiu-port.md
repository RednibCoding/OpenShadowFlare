# Wii U port

The Wii U build compiles the small C99 ShadowFlare runtime and packages it as an
Aroma-compatible Homebrew Bundle (`.wuhb`). It does not include retail game data.

## Install the toolchain

Install devkitPro's Wii U toolchain and Ninja:

```bash
sudo dkp-pacman -S --needed wiiu-dev ninja
```

Set `DEVKITPRO` if devkitPro is not installed at `/opt/devkitpro`.

## Build

From the repository root:

```bash
sh tools/wiiu/build-wuhb.sh
```

The bundle is written to:

```text
build/wiiu/release/OpenShadowFlare.wuhb
```

## Move the game files

The retail data is loaded from the SD card and is never bundled. Copy your
ShadowFlare `System` folder and `SFlare.Cfg` to:

```text
sdcard/wiiu/OpenShadowFlare/ShadowFlare/
  SFlare.Cfg
  System/
  ...
```
