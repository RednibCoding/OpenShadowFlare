# Nintendo Switch port

The Switch build compiles the small C99 ShadowFlare runtime and packages it as a
libnx homebrew application (`.nro`). It does not include retail game data.

## Install the toolchain

Install devkitPro's Switch toolchain and Ninja:

```bash
sudo dkp-pacman -S --needed switch-dev ninja
```

Set `DEVKITPRO` if devkitPro is not installed at `/opt/devkitpro`.

## Build

From the repository root:

```bash
sh tools/switch/build-nro.sh
```

The application is written to:

```text
build/switch/release/OpenShadowFlare.nro
```

## Move the game files

The retail data is loaded from the SD card and is never bundled. Copy your
ShadowFlare `System` folder and `SFlare.Cfg` into a `ShadowFlare` folder at the
SD root:

```text
sdmc:/ShadowFlare/
  SFlare.Cfg
  System/
  ...
```

On an emulator this is the `sdcard/ShadowFlare/` folder inside its data
directory. Copy `OpenShadowFlare.nro` to `sdmc:/switch/` and launch it from the
homebrew menu.
