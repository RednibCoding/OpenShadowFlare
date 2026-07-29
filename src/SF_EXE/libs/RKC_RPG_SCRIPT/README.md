# RKC_RPG_SCRIPT

This is the portable scenario-script boundary. It decodes the retail
`ScenaScriptV000` binary format and owns the small, gradually growing
interpreter used by `SF_EXE`.

The Win32 ABI reconstruction in `src/reconstructed/RKC_RPG_SCRIPT` remains the
reference for the original container and accessor behavior. Executable-owned
opcode behavior is added here only after it has been traced and exercised by
a retail scenario. Game-owned values, such as the local player's level, cross
the boundary through typed queries instead of being stored inside the script
library.
