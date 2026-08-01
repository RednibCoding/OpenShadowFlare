# RKC_RPG_AICONTROL

This is the portable boundary for ShadowFlare's AI action database. It decodes
`Control.aid`, owns its behavior lists and eighteen event buckets, and
provides index and exact-name lookup for executable-owned actors.

The parameter and condition blocks remain indexed arrays until their
individual consumers prove names and units. Selecting actions and carrying
them out belong to the executable actor system, not this container library.
The Win32 reconstruction under `src/reconstructed/RKC_RPG_AICONTROL` remains
the reference for the original storage and accessor behavior.
