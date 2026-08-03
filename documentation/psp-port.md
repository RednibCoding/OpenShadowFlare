# PSP port

## Build

Install [PSPDEV](https://pspdev.github.io/) and expose it in the shell:

```bash
export PSPDEV="$HOME/pspdev"
export PATH="$PSPDEV/bin:$PATH"
psp-gcc --version
```

From the repository root:

```bash
sh tools/psp/build-pbp.sh
```

The release file is `build/psp/release/OpenShadowFlare-psp-release.PBP`.

## Run

Copy the release executable as `EBOOT.PBP`, then place the retail data beside
it:

```text
build/psp/release/OpenShadowFlare-psp-release.PBP -> <memstick>/PSP/GAME/OpenShadowFlare/EBOOT.PBP
<memstick>/PSP/GAME/OpenShadowFlare/SFlare.Cfg
<memstick>/PSP/GAME/OpenShadowFlare/System/...
```

Use a 64 MB PSP model: the release PBP requests the Slim/Go memory layout
(`MEMSIZE=1`), so select PSP-2000 or newer in PPSSPP. The analog stick moves
the cursor; Cross and Circle are primary and secondary click; Start confirms;
Select cancels. Character names use the PSP on-screen keyboard.

The port runs at 333 MHz and mixes audio in mono at 11.025 kHz to reduce CPU
and memory use.
