# PlayStation Vita

Build with [VitaSDK](https://vitasdk.org/) on Linux or WSL:

```sh
sh tools/vita/build-vpk.sh
```

Output: `build/vita/release/src/SF_EXE/OpenShadowFlare.vpk`. GitHub Actions
uploads the same file as `openshadowflare-vita-release`.

The VPK does not include retail data. Copy game installation to:

```text
ux0:data/OpenShadowFlare/ShadowFlare/
```

It must include `SFlare.Cfg` and `System/Title/Pattern/Title.njp`.

## Controls

| Vita control | Game input |
| --- | --- |
| Front touchscreen | Pointer and primary click |
| Left stick | Virtual pointer |
| Cross / Start | Confirm; Cross also primary click |
| Circle / Select | Back; Circle also secondary click |
| D-pad | Menu navigation |
| Triangle / Square | Inventory / map |
| L / R | Mission list / run toggle |

The port uses double-buffered software scaling at 960x544 and the system IME
for character names. Audio is not implemented yet.
