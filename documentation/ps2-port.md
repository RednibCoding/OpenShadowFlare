# PlayStation 2 port

The PS2 port is packaged through Docker, so the repository does not need a
local ps2dev toolchain. With the original game
files present under `tmp/ShadowFlare`, run:

```sh
sh tools/ps2/build-iso.sh
```

The first run builds the `openshadowflare-ps2` image from
`tools/ps2/Dockerfile`; later runs reuse it. The generated disc files,
including `openshadowflare.iso`, are written to `build/ps2`.

Use `--build-image` to force a rebuild of the Docker image after changing the
toolchain setup, and `--data-dir` or `--out-dir` to override the default input
or output locations.
