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
  smoke-animation scheduling, CAF decoding, and steam drawing
- portable V001/V003 VOC decoding and LAL playback for title and menu audio
- new-character and saved-game selection enter/leave behavior
- the character-selection fade and top-level per-frame screen dispatcher
- saved-game list navigation, slot hit regions, double-click timing, and the
  Continue, Delete, Back, and Exit decisions
- new-character gender selection and portable 15-byte name entry
- Online/Single and New/Join mode menus plus portable host-address entry
- saved-game summary parsing, BMP previews, and Delete confirmation
- software drawing with the original Select and bitmap-font pattern sheets
- the six-slot save-name search used by both menu states
- both retail menu input-binding tables
- the statically linked Visual C++ random-number generator
- gameplay entry and its retail loading-screen sub-state
- portable RCLIB-L decoding shared by NJP and ground-map data
- the initial `00000000` scenario's `f00_01` ground and object maps, NJP/SDW
  pattern list, new-player spawn, and player CAF/NJP/SDW drawing

These pieces live in `OpenShadowFlare::GameCore` and have no dependency on
LWL, LGL, LAL, Win32, or another platform API. The executable runtime loads
the config before creating its LWL window, just as the retail entry point does.

The menu lifecycle code emits resource, input, cursor, and audio work through
portable callbacks. Those callbacks deliberately describe what the game needs,
not how a particular operating system provides it.

The title screen's game decisions from `0x00420e60` are now reconstructed.
Its original background, menu entries, selection brightness, and fades are
drawn by the portable runtime. All ten smoke animations use their original CAF
frames, random delays, pipe positions, and scene brightness. Version/copyright
text and network messages are the remaining title presentation work. The
runtime also loads the retail common-effects and title-music VOC containers,
plays voice 62 for the title cue, voice 58 for navigation, voice 56 for title
confirmation, and starts the looping BGM after the original 60-frame delay.

The top-level character-selection dispatcher at `0x00421c50` is reconstructed
through its fade, mode dispatch, sub-screen dispatch, and delayed gameplay
transition. New-character creation, the saved-game list, Delete confirmation,
and all shared mode/host screens are implemented and drawn with the original
assets. LWL supplies portable UTF-8 text and clipboard input; the game state
keeps the retail 15-byte field limit and does not depend on a native edit
control. Selecting a new character now follows the executable's exact
20-frame, 97-pixel center slide while the opposite portrait fades. The reverse
animation, 130-by-20 name field, and 6-by-12 block caret mirror the retail draw
packets as well. Shared mode dialogs are drawn as full-bright overlays after
their dimmed backdrop, and returning from the saved-game dialog restores both
the list brightness and its interaction state. Save-slot numerals now remain
static unless pointed at; only the hovered numeral follows the retail
3-2-1-0-0-1-2-3 pulse.

Character selection keeps the title BGM playing when it is already active,
starts it on entry when necessary, uses common voice 55 for its selection
actions, and releases the music when the menu flow ends. All playback uses the
retail effect and BGM volume values through LAL.

Gameplay entry at `0x0041d3f0` now prepares the initial single-player world.
The initial loading presentation follows the cached `Waiting.njp` path in
`0x00402920`: pattern 0 is the Episode 1 background, pattern 3 is the
bottom-right loading label, and pattern 2 becomes the horizontally moving
confirmation arrow after world setup completes. Return or a click inside the
retail arrow rectangle enters the world. The 120-frame `VisualNN.njp` fade in
`0x00417bd0` is a separate loading presenter used later in gameplay and is not
part of this initial transition.

The first scenario then draws the decoded `f00_01.Gnd` cells at the new-player
entry record (`89898`, `2811`, direction `3`) and places the selected male or
female animation at the camera center. Its 279 `f00_01.Obl` records now supply
Remote Town's gates, walls, trees, rocks, and other static scenery. Pattern
bounds provide view culling, the OBL status classes and judgement rectangles
provide retail depth keys, and the player is inserted into the default scenery
pass at foot depth. Paired `ShadowLowPat` SDW assets render first with the
configured opaque or 50-percent shadow mode through GAPI's general opacity
support.

The player CAF path now follows `0x00434ef0` and the appearance refresh at
`0x00444ca0`: the MCT direction is preserved, and the per-part enable table
starts with only the base body and shadow. Equipment layers remain disabled
until the future inventory slice has an equipped item to select them. The
player shadow comes from
`Animation00.Sdw`; palette index zero remains the NJP cutout rather than being
drawn as a black pixel.

The initial scenario's MCT music field is `0`. The transition helper at
`0x004275e0` maps that to `System\Game\Music\BGM00.Voc`, and the update helper
at `0x004275a0` starts sample zero looping in voice slot 500. Gameplay now
starts the same track once the world is prepared, applies the configured BGM
volume through LAL, and releases it with the gameplay state.

This is still not complete gameplay rendering. The HUD, movement, NPCs, world
simulation, scripting, darkness, equipment state, and saved-game scenario
restoration are the next executable layers.
