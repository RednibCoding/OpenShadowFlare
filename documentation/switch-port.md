# Nintendo Switch port

OpenShadowFlare builds as a Nintendo Switch homebrew application (`.nro`) via
libnx. The NRO does not include the original game data.

## Build locally

Install devkitPro's Switch environment (`devkitA64`, `libnx`, and
`switch-tools`). In the devkitPro MSYS2 shell or a POSIX shell on Linux, run from the repository root:

```bash
sh tools/switch/build-nro.sh
```

The script uses devkitPro's Switch CMake toolchain and Ninja, configures a
release build, disables host-only tests, and writes the result to:

```text
build/switch/release/src/SF_EXE/ShadowFlare_rebuilt.nro
```

If `build/switch/release` was previously configured with another CMake
generator, remove it once before running the script:

```bash
rm -rf build/switch/release
```

## GitHub Actions

`Build Nintendo Switch NRO` runs the same script in the pinned devkitPro
container and uploads `openshadowflare-switch-release` as a workflow artifact.
Download that artifact to test a CI build; no local devkitPro or Android Studio
installation is required.

## Add game data

On Switch hardware, place the NRO beside a `ShadowFlare` directory:

```text
sdmc:/switch/OpenShadowFlare/
  ShadowFlare_rebuilt.nro
  ShadowFlare/
    SFlare.Cfg
    System/
      ...
```

```text
<Emulator data folder>/sdcard/switch/ShadowFlare/
  SFlare.Cfg
  System/
    ...
```

Load the NRO with **File > Load Application**

## Controls

| Switch input | Game action |
| --- | --- |
| Touchscreen | Pointer movement and primary click |
| D-pad | Menu navigation and movement |
| A / Plus | Confirm |
| B / Minus | Back / cancel |
| X | Inventory |
| Y | Map |
| L | Mission list |
| R | Toggle walk/run |
| ZL | Open the character-name keyboard |
