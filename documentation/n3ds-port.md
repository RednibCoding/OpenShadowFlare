# Nintendo 3DS port

OpenShadowFlare builds as a Nintendo 3DS homebrew application (`.3dsx`). It
targets New 3DS systems first and does not include the original game data.

## Build locally

Install devkitPro's 3DS environment (`devkitARM`, `libctru`, and `3ds-tools`).
From the devkitPro MSYS2 shell on Windows, or a POSIX shell on Linux, run:

```bash
sh tools/n3ds/build-3dsx.sh
```

The result is:

```text
build/n3ds/release/src/SF_EXE/OpenShadowFlare.3dsx
```

## Run on an emulator or on hardware

```text
3ds/OpenShadowFlare/
  ShadowFlare/
    SFlare.Cfg
    System/
      ...
```

Load `OpenShadowFlare.3dsx` or place it in the emulator's roms folder to test it. On hardware, launch it from
the Homebrew Launcher. The game reads and saves files in the external
`ShadowFlare` directory; no game data is packed into the 3DSX.

The game is rendered on the bottom touchscreen. During active gameplay, the
top display shows a compact live minimap centered on the player. It is hidden
on title, character-select, loading, and scenario screens.

## Controls

| 3DS input | Game action |
| --- | --- |
| Touchscreen | Pointer movement and primary click |
| Circle Pad / D-pad | Menu navigation and movement |
| A | Confirm |
| B | Back / cancel |
| X | Inventory |
| Y | Map |
| L | Mission list |
| R | Toggle walk/run |
| ZR | Magic |
| Select | Help |
| ZL | Open the character-name keyboard |
| Start | Exit the game |

## GitHub Actions

`Build Nintendo 3DS 3DSX` runs the same build in devkitPro's devkitARM
container and uploads `openshadowflare-n3ds-release` as a workflow artifact.
