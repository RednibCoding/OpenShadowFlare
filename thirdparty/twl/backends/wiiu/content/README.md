# Wii U bundle content

Files in this directory are bundled into `OpenShadowFlare.wuhb` and mounted at
`/vol/content` on the console.

The Wii U TWL backend loads `texture_blit.gsh` from here at startup. It is the
committed compiled output of `../shaders/texture_blit.{vsh,psh}`; see
`../shaders/README.md` to regenerate it.
