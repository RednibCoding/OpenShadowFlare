# Portable DLL libraries

This directory contains the clean, cross-platform counterparts of behavior
first reconstructed in ShadowFlare's DLLs. Each subdirectory corresponds to
one original DLL and builds as a static library for `SF_EXE`.

The reconstructed Win32 DLL sources remain under
`src/reconstructed/<DLL name>` as the behavioral and ABI reference. Code here
preserves the behavior that the portable executable needs, but uses native
ownership, standard C++ types, and no original object layout or platform API.

All fourteen DLL boundaries have a matching subdirectory. A static target and
public header are added when an executable slice first needs that DLL's
behavior; until then its directory records the intended boundary. Implemented
libraries expose one public header named after their DLL and split the
implementation into small files by concern.

DLL-derived portable implementation belongs in this directory, never directly
in `GameCore` or another `SF_EXE` folder. The `source_boundaries` native test
checks the fourteen matching directories, the implemented public APIs, and the
known ported implementation units so accidental boundary regressions fail in
CTest.
