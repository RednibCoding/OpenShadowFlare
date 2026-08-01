# PSP port

## Build

Extract the pspdev release, then make its tools available:

```bash
export PSPDEV="$HOME/pspdev"
export PATH="$PSPDEV/bin:$PATH"
psp-gcc --version
```

Configure and build from the repository root:

```bash
cmake -S . -B build/psp/release -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PSPDEV/psp/share/pspdev.cmake" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/psp/release -j"$(nproc)"
```

The PSP configuration selects the `gu` presenter and disables native tests
automatically. The output is:

```text
build/psp/release/src/SF_EXE/EBOOT.PBP
```

## Run in PPSSPP

Copy the EBOOT and retail data into the same game directory:

```text
<memstick>/PSP/GAME/OpenShadowFlare/EBOOT.PBP
<memstick>/PSP/GAME/OpenShadowFlare/SFlare.Cfg
<memstick>/PSP/GAME/OpenShadowFlare/System/...
```

Set PPSSPP to a 64 MB PSP model, then launch `EBOOT.PBP`.
