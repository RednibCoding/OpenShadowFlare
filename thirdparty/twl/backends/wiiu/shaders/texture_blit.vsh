; Wii U texture-blit vertex shader (Latte assembly for latte-assembler).
;
; Passes a full-screen quad's clip-space position straight through and forwards
; the texture coordinate to the pixel shader. Attribute register mapping follows
; the convention "attribute at location N arrives in register R(N+1)".
;
;   aPosition (location 0) -> R1  -> POS0
;   aTexCoord (location 1) -> R2  -> PARAM0 (semantic 0)
;
; Compiled to texture_blit.gsh with latte-assembler; see shaders/README.md.

; $MODE = "UniformRegister"
; $ATTRIB_VARS[0].name = "aPosition"
; $ATTRIB_VARS[0].type = "vec4"
; $ATTRIB_VARS[0].location = 0
; $ATTRIB_VARS[1].name = "aTexCoord"
; $ATTRIB_VARS[1].type = "vec2"
; $ATTRIB_VARS[1].location = 1
; $NUM_SPI_VS_OUT_ID = 1
; $SPI_VS_OUT_ID[0].SEMANTIC_0 = 0

00 CALL_FS NO_BARRIER
01 EXP_DONE: POS0, R1.xyzw
02 EXP_DONE: PARAM0, R2.xyzw NO_BARRIER
END_OF_PROGRAM
