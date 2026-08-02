# Wii U port

The Wii U build produces an Aroma-compatible Homebrew Bundle (`.wuhb`). It
does not include retail game data.

## Build

Install devkitPro's Wii U toolchain, SDL2 port, and Ninja:

```bash
sudo dkp-pacman -S --needed wiiu-dev wiiu-sdl2 ninja
```

From the repository root, build the bundle:

```bash
sh tools/wiiu/build-wuhb.sh
```

Output:

```text
build/wiiu/release/OpenShadowFlare.wuhb
```

```bash
rm -rf build/wiiu/release
```

## Test in Emulator

```text
sdcard/wiiu/OpenShadowFlare/ShadowFlare/
  SFlare.Cfg
  System/
  ...
```

Load `build/wiiu/release/OpenShadowFlare.wuhb`. Configure Controller as a **Wii U GamePad**;
a Pro Controller profile does not provide the VPAD input used by this port.

## Controls

| GamePad control | Game action |
| --- | --- |
| Touchscreen | Pointer and primary click |
| D-pad | Menu navigation and movement |
| A / Plus | Confirm |
| B / Minus | Back / cancel |
| X | Inventory |
| Y | Map |
| L | Mission list |
| R | Toggle walk/run |
| ZL | Enter `Player` in the name field |

The current SDL2 presenter mirrors the game to the TV and GamePad.
