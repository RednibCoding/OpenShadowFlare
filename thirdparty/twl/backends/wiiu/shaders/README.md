# Wii U blit shaders

`texture_blit.vsh` / `texture_blit.psh` are the Latte-assembly source for the
full-screen texture blit used by the Wii U TWL backend. They are compiled once
into `../content/texture_blit.gsh`, which is committed, bundled into the `.wuhb`,
and loaded at runtime with `WHBGfxLoadGFDShaderGroup`.

The repository build and CI **do not** run the shader compiler; they only bundle
the committed `.gsh`. You only need the tool below if you change a shader.

## Regenerating texture_blit.gsh

The compiler is `latte-assembler` from decaf-emu. Only a Windows binary is
published (<https://github.com/decaf-emu/latte-assembler/releases>), so on Linux
run it through Wine:

```sh
wine latte-assembler.exe assemble \
  --vsh texture_blit.vsh \
  --psh texture_blit.psh \
  ../content/texture_blit.gsh
```

Then commit the regenerated `../content/texture_blit.gsh`.

### Notes for this compiler version

The published `v0.1` binary is older than the assembly examples shipped with
current wut, so:

- attribute/uniform types use **GLSL names** (`vec4`, `vec2`, ...), not
  `Float4`/`Float2`;
- keep the metadata minimal (attribute vars + `SPI_VS_OUT_ID` /
  `SPI_PS_INPUT_CNTL`). The verbose `SQ_PGM_RESOURCES_*`, `SQ_VTX_SEMANTIC_*`
  and `VGT_*` fields from the newer examples are rejected by this build.

## Status

The shader assembles cleanly and renders the title screen correctly under Cemu.
If a future screen looks vertically flipped, flip the `V` texture coordinates in
the backend quad in `presentation.c`, not the shader.
