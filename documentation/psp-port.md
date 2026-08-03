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

## Run in PPSSPP

Copy the release executable as `EBOOT.PBP`, then place the retail data beside
it:

```text
build/psp/release/OpenShadowFlare-psp-release.PBP -> <memstick>/PSP/GAME/OpenShadowFlare/EBOOT.PBP
<memstick>/PSP/GAME/OpenShadowFlare/SFlare.Cfg
<memstick>/PSP/GAME/OpenShadowFlare/System/...
```

Use the 64 MB PSP model. The analog stick moves the cursor; Cross is the
primary click, Circle is secondary click, Start confirms, and Select cancels.
The PSP on-screen keyboard is used for character names. Audio is mixed at
22.05 kHz and duplicated to the PSP's fixed 44.1 kHz hardware output.

## PSPDEV portability

PSPDEV defines `std::int32_t` as `long`, while literals such as `0` are `int`.
`std::min`, `std::max`, and `std::clamp` require all operands to have the same
type. For integer expressions involving `std::int32_t`, use an explicit type:

```cpp
std::clamp<std::int32_t>(value, 0, upper);
```

Alternatively, type the literal: `std::max(value, std::int32_t{0})`. Do not
apply the integer template argument to floating-point expressions.
