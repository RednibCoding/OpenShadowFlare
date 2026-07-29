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
- the initial `00000000` scenario's fixed MCT header and entry-point table
- its leading ID lists, common variable entity record, and seven-record
  `PEOPLE` block
- its data-selected ground and object maps, NJP/SDW pattern list, new-player
  spawn, facing direction, title, music, and player CAF/NJP/SDW drawing
- Ostare's retail people resource, placement, part mask, shadow, idle pause,
  and bounded walk
- the complete Remote Town SCS container and the first status-triggered
  conversation, including asynchronous message-close callbacks, native actor
  hooks, four script-created ground items, and the two-message player-level
  branch used on a repeat interaction
- Ostare's pointer hover, pale selection tint, nameplate, and actor-anchored
  `Hukidasi.njp` speech bubble
- judgement-rectangle interaction range and retail-style auto-approach before
  an NPC status starts
- one shared executable-owned movement controller for the player and PEOPLE
  actors, including persistent cardinal obstacle-edge steering and the retail
  ownership later enemies will also use
- smooth 60 Hz camera and actor presentation interpolated from unchanged
  30 Hz gameplay updates
- companion choice messages with hidden `~` range markup, pointer hit testing,
  persistent red hover selection, gray inactive choices, script result
  write-back, initial-minus-one informational follow-ups, and status-one
  callbacks, verified through Kerberos and Harley's live Remote Town
  interactions
- opaque-pixel world selection for PEOPLE actors and ground items, including
  configured click priority, the retail range square, item nameplates and
  pale tint, shared auto-approach, and the first inventory-owned pickup

These pieces live in `OpenShadowFlare::GameCore` and have no dependency on
LWL, LGL, LAL, Win32, or another platform API. The executable runtime loads
the config before creating its LWL window, just as the retail entry point does.

The first gameplay HUD layer now follows `0x004039f0`. `Bar.njp` supplies the
fixed lower interface, walk/run marker, level digits, life and mana fills, and
experience frame. GAPI has a general destination clip for the two live fills,
so the original 206-pixel artwork is revealed rather than stretched. The HUD
is a screen-space renderer outside the world camera and owns the lower input
band. Retail registers the standard Windows arrow once and never calls
`SetCursor`; LWL's native platform arrow is therefore the portable equivalent.

World feedback now follows `0x004165d0`, `0x0040ee70`, and `0x00416bb0`.
Portable `RKC_RPGSCRN` tests the configured inclusive square against the
opaque pixels of each current visible CAF/NJP cell. `WorldPointer` prefers an
exact tip hit, otherwise the nearest candidate in a priority group, while
retaining retail display depth and the five priorities from `SFlare.Cfg`.
Disabling the range reduces selection to the exact cursor tip. The default
priority selects an item over a person. The range renderer uses the same exact
half-sizes 0, 12, 16, 24, and 48, with strength 100 over empty ground and 300
over a target. PEOPLE targets are white and item targets are yellow.

Ground items use their `Item.Ibn` name, while money shows its quantity followed
by `Gold`. Their visible part receives the same +300 pale tint as a selected
person. Type-three interaction in `0x00449240` shares the 159-unit rectangle
range and movement-controller approach. Once close, the first single-player
`0x004526a0` path transfers the concrete item to `PlayerInventory` and erases
the stable ground entity only after acceptance. Gold stacking to 10,000 is
covered; grid placement, item footprints, full-inventory handling, panel
drawing, and network replication remain.

The in-game settings panel now follows `0x004103c0`. Escape opens the original
two-layer `Status.njp` panel and suspends world input. Boolean options use the
retail ON/OFF cells, click range uses its five 24-pixel cells, clicking a
priority class moves it to the right-hand end exactly as the executable does,
and both volume sliders use the original mute sentinel and `-3000` through
zero scale. Pointer, shadow, occluding-object, effect-volume, and BGM-volume
changes apply live; all changed configuration fields are written back through
the reconstructed 64-byte `SFlare.Cfg` writer when the game exits. The
screen-mode row is hidden and the LWL window stays windowed, but y=86 remains
empty so every later row keeps its retail coordinate. Mission, map, and help
remain placeholders.

The two save rows now use the same secondary states as `0x004103c0`. Their
prompts replace the settings text without replacing or re-fading the panel;
YES and NO retain their retail coordinates, hitboxes, hover color, and samples
56 and 55. YES calls the portable `0x0044b580` path before either returning to
the title or ending the process. A successful write shows the retail
`Now saving the data ` stage for one update; a failed write leaves the
confirmation open.

The writer reproduces the `ShadowFlare0005` envelope: plain 0x160-byte menu
record, payload size, one-byte Visual C++ `rand()` XOR key, signed-byte
checksum, and the inverse of the executable's 256-byte substitution table.
When rewriting an original save, it validates and decodes the payload, updates
its repeated player record, and preserves every still-unmapped byte before
encoding it again. New characters currently write the owned player record as
their first payload slice. Scenario, position, inventory, equipment, and quest
serialization still need to be added as their corresponding load paths are
reconstructed. Writes go through a sibling temporary file and protected
replacement so a corrupt source or failed write does not silently destroy the
slot.

With `Save Image at Game End` enabled, the world-only software surface is also
captured before the HUD, conversation overlay, or Escape panel is drawn.
Portable `RKC_DIB` writes the retail 391-by-114, 24-bit `Save\%04d.Bmp` partner
from the player-centered crop. The saved-game screen reads that file through
the same portable BMP boundary and displays it at the original coordinates.

Portable behavior originating in the DLLs is kept under `SF_EXE/libs`, with a
directory for each of the fourteen original boundaries. `RK_FUNCTION`,
`RKC_DBFCONTROL`, `RKC_DIB`, `RKC_DSOUND`, `RKC_UPDIB`, `RKC_RPGSCRN`, and
`RKC_RPG_SCRIPT` currently build as separate static libraries. Each has one
public API header and small implementation files; future executable slices
should port proven behavior from the corresponding Win32 reconstruction into
the matching library instead of adding it directly to `GameCore`.

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

`ScenarioData` follows the fixed part of `0x00427b50`: it validates
`MCED DATA v0000\x1a`, reads the two 260-byte paths, music index, and 256-byte
area title, then decodes the trailing 16-byte entry records in their file order
of key, world X, world Y, and direction. At `0x324`, the first three counted ID
lists lead into a shared variable entity record. The portable slice skips the
understood-size object tail and decodes the following `PEOPLE` records,
including variable names and custom part/color arrays. Later entity groups
remain opaque. For scenario `00000000`, entry key zero supplies
(`89898`, `2811`, direction `3`) and the MCT map path selects `f00_01`.

The first scenario then draws the decoded `f00_01.Gnd` cells at that entry and
places the selected male or female animation at the camera center. Its 279
`f00_01.Obl` records now supply
Remote Town's gates, walls, trees, rocks, and other static scenery. Pattern
bounds provide view culling. Static scenery, people, ground items, and the
player now enter combined shadow and visible lists. The initial
`InsertSort` order uses status class and the projected left/top judgement
corner; `SortDisplayObject` then reproduces the retail strict comparison of
all four absolute rectangle edges. This full pass fixes long Remote Town wall
segments and both city houses occluding an actor behind them. Paired
`ShadowLowPat` SDW assets use the same ordering and the configured opaque or
50-percent shadow mode through GAPI's general opacity support.

The first decoded person is Ostare: local ID `0`, people resource `13`, name
color `0x00e0e0e0`, label height `80`, position (`91467`, `1532`), judgement
`[-80, -80, 79, 79]`, and direction `7`. The custom 256-entry part table leaves
CAF parts 0, 1, 2, 3, and 6 enabled while disabling 4 and 5. Portable loading
resolves that resource to `Character/PEOPLE/00000013`, loads its NJP, SDW, and
CAF, and runs it at the 30 Hz game cadence. The people tail supplies speed 10,
a 30-update walk limit, a 30-update idle pause, and a spawn-relative rectangle
from (`-437`, `-223`) to (`269`, `231`). The type-one update at `0x0045d150`
starts movement after that pause; movement-controller mode three chooses an
inclusive random point in the rectangle, and `0x0045d9f0` selects CAF chart
one until arrival or the walk limit. The first tested draw uses shadow pattern
280 and visible patterns 1744 and 1784 at the retail starting camera anchor.
Actor shadows and visible cells share the depth-sorted world passes with the
player and static OBL scenery.

The same scenario's `Scenario.Scs` is now decoded in full: version `000`, 66
temporary flags, no network flags, 61 bitwise-inverted messages, 23 status
triggers, 220 sentences, and 608 commands. The interpreter entry at
`0x00430f80` is reconstructed for the commands exercised by Ostare's complete
opening interaction: conditional sentence calls, assignments, messages,
addition, subtraction, item creation, and three native actor actions. Clicking
local person zero derives character number `12000000`, resolves status kind
zero to sentence four, and shows retail message `1000000`. Opcode 2 finishes
the rest of its sentence before returning a message wait. Closing the actor
bubble follows `0x00453220` into status kind one at `0x00430940`; repeated
callbacks show messages `1000001` through `1000004` and finally release the
actor through opcode 19.

Scenario startup now instantiates all seven records in the Remote Town PEOPLE
group instead of stopping after the first. Their resource IDs resolve through
the same zero-padded `Character/PEOPLE` catalog, including the shared
`01000000` and `01000001` animal resources. Malse's new-game status follows
messages `1000019` and `1000020`; the later quest offer stays behind the
retail Red Goblin progression check.

Syria's new-game status follows messages `1000040` and `1000041`. Opcode 2 now
passes the current script character through the message event, which anchors
her bubble without inventing an actor command absent from this branch.
`0x00433f29` evaluates three operands and updates quest state. Its ordinary
path writes the requested value and sends event `0x41`; its state-two path
latches completion, requires old state one, writes state two, and sends event
`0x42`. The optional server broadcast remains outside the current
single-player slice. Syria exercises `{quest 0, state 1, no broadcast}`.
`0x00433868` then stores quest zero as the selected notice and writes `600` to
the adjacent counter. The counter's consumer and the two cue sounds remain to
be traced.

The callback for message `1000003` reads Ostare's live position through
operand types 6 and 7. Opcodes 11 and 12 form the offsets, and four opcode-10
calls create three ordinary ground-item records plus 200 money. The portable
records preserve category, definition ID, quantity, and retail position.
`0x00462f80` is reconstructed as an executable-owned item database loader:
the 256-byte substitution table, RCLIB-L block, signed-byte checksum, five
category counts, inverted names and descriptions, and category-specific
record sizes are all checked against the retail file. The `Item0000` group
and patterns 0, 45, 279, and 270 are inventory artwork only. The ground-item
path at `0x00458930` instead reads definition offsets `0x30` and `0x34`,
loading Character/ITEM resource zero and CAF charts 0, 5, 36, and 30. Those
charts produce NJP patterns 77, 82, 113, and 107 with their SDW shadows. The
CAF's mode-one palette rule selects `chart * 2 + cell priority`, and
definition offsets `0x3c`, `0x40`, and `0x44` provide the visible part's
default red, green, and blue strengths. The
portable entities also reproduce the 1600 initial vertical velocity, 280
gravity, 700 rebound, and two-bounce settle state before joining the shared
dynamic depth pass. Unnamed definition fields remain preserved as raw bytes.

On the next click, the retained type-11 flag enters sentence six. Opcode 61 at
`0x00433f16` reads offset `0x34` from the local player, writes level one to the
script destination, and selects the under-level-five message `1000005`. Its
callback shows `1000006` before releasing Ostare. Unknown opcodes stop with an
explicit unsupported result.

Pointing at Ostare follows `0x0040ee70`: his projected actor bounds select the
person, his MCT label height places the half-transparent black nameplate, and
the renderer adds 300 to each visible part's color strength. RKC_UPDIB
strengths above 1000 move palette channels toward white, so this produces the
retail pale tint rather than merely making dark pixels brighter. His opening
message is measured as Shift-JIS-aware 6-by-12 text by the rules at
`0x00456550`; `0x00456bb0` builds the actor-relative frame and tail from
`Hukidasi.njp`. A retail-data render test covers the nameplate, tint, frame
pieces, dimensions, and anchor coordinates.

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

The first interactive world loop follows the retail movement path through
`0x00441c00`, `0x00454210`, `0x00454930`, `0x004351f0`, and `0x00434ef0`.
LWL button state becomes a 640-by-480 ground command, the RKC_RPGSCRN inverse
projection turns it into a world destination, and the unequipped new player
gets speed tier five. `0x00450080` indexes the retail factor table, multiplies
its `1.0` result by the base speed `20.0`, and stores a 20-unit walk step plus
the doubled 40-unit run step. Retail's DBF thread advances movement roughly
every 33 ms. The portable shell now uses an elapsed-time accumulator to update
all game state at the same 30 Hz cadence while continuing to present at 60 Hz.
This keeps movement and CAF frame counters on one clock and preserves their
real-time speeds. A click remains a latched destination. Held input replaces
that destination without restarting the current action. The input record's
hold counter must pass nine updates before release cancels movement, matching
the test at `0x00441c00`; an ordinary multi-update click therefore still
auto-moves.
Speech-bubble clicks remain consumed until their button release, including
when an option closes the conversation on the press frame.

Walk is executable action 2 and CAF chart 1 at `0x004351f0`; run is action 3
and CAF chart 2 at `0x00435530`. The `R` binding now toggles the persistent
movement mode on its key-down edge. Switching while already moving resets the
animation counter and immediately changes both the chart and the movement
step, as the retail action transition does.

GND loading now includes the second, 852-by-852 Remote Town judgement plane.
The portable RKC_RPGSCRN boundary checks its bit-zero blockers and the
status-one OBL judgement rectangles against the retail player box
`[-80, -80, 79, 79]`. The movement controller performs integer swept checks,
keeps the last walkable point, and preserves the retail movement/wall
direction state. The collision sweep follows the dominant axis and uses the
same integer interpolation as `0x00414990`; cardinal edge movement also keeps
the resolver's one-pixel side contact. On first contact, `0x00454930` selects
its movement/wall pair from the attempted quadrant, returned contact, and
one-pixel position probes. Its exit and corner changes use the original
destination-sign tables rather than a distance-improvement guess.

There is no A* fallback in this path. Player and PEOPLE actors share the same
controller and face their actual detour step. Live town actors contribute
their judgement rectangles, including the actor being approached. The
159-unit rectangle range ends an interaction approach before that target
becomes a collision. Fixtures cross the sacks beside Ostare, cover the exact
Ostare-to-Malse route with live actors present, and use successive ordinary
movement legs for longer trips to companion interactions. The renderer
reads chart zero for idle and chart one for walking directly from player state,
rebuilds the depth key from the moving position, and follows the player's
projected position with the retail camera offset.

This is still not complete gameplay. The other people records, broader AI,
dynamic collision for NPC and enemy movement, remaining script commands and
operand domains,
alternate conversation modes, HUD, darkness, equipment state, and saved-game
scenario restoration are the next executable layers.
