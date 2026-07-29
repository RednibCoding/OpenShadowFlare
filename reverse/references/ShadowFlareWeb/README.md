# ShadowFlareWeb historical reference

This is an incomplete ShadowFlare reconstruction written in C with
SDL2. It is not part of the OpenShadowFlare build and it is not a faithful or
bug-free implementation.

It is still a useful research reference. In particular, it already explores
areas such as:

- moving between maps;
- combat;
- the teleport spell;
- AI;
- scenario and script execution;
- several original resource formats.

Use those implementations to find promising retail code and data, not as
proof that a behavior is correct. Anything brought into the current portable
reconstruction should first be checked against `ShadowFlare.exe`, the original
assets, and the tested reconstructed DLLs, then rewritten to fit the current
architecture.
