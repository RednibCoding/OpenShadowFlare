# Reconstruction status labels

- `identified`: the address and broad purpose are known.
- `analyzed`: control flow and important side effects are understood.
- `partial`: some corresponding portable behavior is implemented, but it is
  not complete enough for a fidelity claim.
- `implemented`: the portable behavior is complete enough for focused tests.
- `verified`: behavior has passed an original-versus-reconstruction test with
  representative retail data or event sequences.

Addresses describe the retail executable only. The portable executable does
not try to preserve internal addresses, compiler-generated class layouts, or
operating-system handles.

## Current portable slices

The first game-core slice covers:

- the constructor defaults used when `SFlare.Cfg` is absent
- the exact 16-value, 64-byte config load/save layout
- retail validation, clamping, and partial updates on failed reads
- Shift-JIS-aware `/w` and `/f` command-line handling
- the title, character-selection, and gameplay transition dispatcher
- the title enter/leave lifecycle and its exact resource manifest
- the title fade, menu navigation, hover regions, delayed actions, audio cues,
  and smoke-animation scheduling
- new-character and saved-game selection enter/leave behavior
- the character-selection fade and top-level per-frame screen dispatcher
- the six-slot save-name search used by both menu states
- both retail menu input-binding tables
- the statically linked Visual C++ random-number generator

These pieces live in `OpenShadowFlare::GameCore` and have no dependency on
LWL, LGL, LAL, Win32, or another platform API. The executable runtime loads
the config before creating its LWL window, just as the retail entry point does.

The menu lifecycle code emits resource, input, cursor, and audio work through
portable callbacks. Those callbacks deliberately describe what the game needs,
not how a particular operating system provides it.

The title screen's game decisions from `0x00420e60` are now reconstructed.
Drawing is still pending, but the portable runtime already feeds LWL input
into the original selection rules and follows the resulting state changes.

The top-level character-selection dispatcher at `0x00421c50` is reconstructed
through its fade, mode dispatch, sub-screen dispatch, and delayed gameplay
transition. The next layer is the mode-specific interaction inside
`0x00422e30` and `0x00423ca0`, followed by the sub-screens at `0x00424d80`,
`0x004253c0`, `0x00425830`, and `0x00425d40`.
