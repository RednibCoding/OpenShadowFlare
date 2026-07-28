# Reconstructed compatibility DLLs

This folder contains the faithful Win32 reconstructions of ShadowFlare's
fourteen original DLLs. They preserve the original exports and behavior closely
enough to be loaded by the retail `ShadowFlare.exe` under Windows or Wine.

These sources are the tested behavioral reference for the portable work in
`src/SF_EXE/libs/`. They are intentionally kept separate: code needed by the
portable executable is cleaned up, made cross-platform, and placed in the
matching static-library folder there instead of being included directly from
this compatibility layer.

Run `./src/build.sh` from the repository root to cross-compile all fourteen
DLLs. The output remains in `src/build-win32/`; use `./src/build.sh --deploy` to
back up the original game DLLs and copy the reconstructed ones into the local
game directory.
