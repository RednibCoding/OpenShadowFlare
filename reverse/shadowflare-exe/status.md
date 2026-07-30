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
covered. Definition offsets `0x1c`, `0x20`, and `0x24` now provide the
inventory width, height, and item weight. Backpack insertion scans the retail
9-by-4 grid in row-major order, retains each multi-cell footprint, and leaves
ownership unchanged when no complete placement is available.

The first inventory screen follows `0x00404760`, `0x00407170`, and
`0x00408a80`. `I` and the lower ITEM button toggle the live right-hand panel,
move the camera anchor to x=160, and restrict world picking to x=0..319.
`Status.njp` patterns 2, 3, 0/1, and 74/75 supply the original frame, gender
silhouette, and Close tab. Gold is summed from the owned stacks; equipped
weight comes from the equipment owner. Backpack
items draw from the separate `Item0000.njp` through `Item0013.njp` groups at
`(336 + grid_x*32, 264 + grid_y*32)`. The inventory and lower HUD regions are
cleared before their transparent authored layers are composed, matching
retail's reserved UI surfaces instead of exposing world pixels through slots.
Moving and dropping use the retail held-item path. All nine equipment
boxes now follow the `0x00447290` regions: helmet `560..623,16..79`, body
`560..623,88..183`, boots `560..623,192..255`, main hand
`480..543,16..143`, off hand `480..543,160..255`, and the four accessory
cells at `(400,143)`, `(400,183)`, `(440,143)`, and `(440,183)`. Category zero
belongs in the main hand. Category one's first serialized field classifies
helmet, body, off hand, and boots as subtypes zero through three. Accessories
accept category two with a one-cell width and use serialized offset 100 for
their required level. Every box applies that check and performs the same
pointer swap. Equipped weight and the ten base contribution fields are summed
over all nine objects.

The HUD pockets follow `0x00445bd0` and the item tail of `0x00407170`. They
form a separate 4-by-2 category-three owner, with row-zero origin `(357,413)`
and row-one origin `(405,445)`. Pointer hit testing uses the exact staggered
rectangles, complete item footprints are retained, and placing over one item
swaps it onto the shared pointer. `0x0044a5f0` maps keys `1` through `8` to
the four row-zero cells followed by the four row-one cells. `0x0044a240`
applies the decoded flat and maximum-percent player life/mana fields and
removes the item only when a value changes. Its companion and status-effect
branches remain pending.

New-character equipment and owned items now follow `0x00440f70` as well.
Category one definition zero is equipped in the body slot. Category-three
definition zero fills backpack column zero and belt row zero four times;
definition `10000000` does the same for backpack column one and belt row one.
The separate mine counter starts at five. This initialization runs only for a
new character, not as a fallback for the still-undecoded save payload.

Inventory movement sounds follow `0x00466110`: category-two items use sample
93, Gold uses 85, ordinary items below weight 60 use 48, and heavier ones use
47. Equipment placement uses sample 49 except for category two, while a
successful belt effect uses sample 16. Sounds are emitted only after the
corresponding ownership change succeeds.

Ground drops use selector two from the same routine. Their first contact with
the ground plays sample 15 for an ordinary item, 85 for Gold, or 93 for a
category-two item. The smaller second bounce is silent. This lives in the
shared ground-item update path, so script-created drops and player-dropped
items behave alike.

The `X` panel follows `0x00404760`, `0x00409000`, and `0x00447970`. It owns a
separate 9-by-10 grid whose first item cell is `(16,72)`, composes Status
patterns 14 and 15 over a reserved left-hand surface, moves the camera anchor
to x=480, and keeps the right-hand world viewport live. It shares pointer
ownership, centered placement, single-item displacement, Gold stacking, and
the three-update information delay with the ordinary inventory. Opening
Inventory or Special Item closes the other. Opcode 41 argument zero reaches
this same owner from the Warehouse; it does not create another container.
The owner now restores and rewrites its exact save-payload container. The
inventory-panel transfer button at classifier case 10 remains pending.

Category-one records place the requirement at serialized offset 148, CAF part
at 152, and default RGB strengths at 156, 160, and 164. The Short Sword
therefore enables part 12, the Round Shield enables part 9 with strengths
900/800/500, and body armor follows the same decoded path. Retail
`0x00444ca0` only adds the body, main-hand, and off-hand objects to the CAF
mask; helmet and boots remain stat-bearing slots without their own enabled
layer. A weapon's optional second part and the retail off-hand suppression
rule are decoded too.

The shared item information path at `0x00409160` and `0x00409a60` is now used
by backpack, equipment, and special-item hovers. The pointer must remain on
the item for three
updates. Font01 uses its native 6-by-12 cells with no extra letter spacing;
the widest elemental row controls the centered x position. Retail flag `0x20`
adds four pixels of padding around the text, fills that rectangle with the
600/1000 black fade, and the four `0x204` packets draw its 500/1000 white
one-pixel frame. The padded rectangle starts eight pixels below the pointer,
and both axes clamp to the 640-by-480 frame. Ordinary items list only non-zero
values from the ten-field contribution vector, then
durability, weight, required level, the condition-adjusted quarter-price, and
the two four-element rows. Serialized offset 20 supplies the base price,
offset 100 supplies maximum durability, and category-zero/category-one
element arrays begin at offsets 208 and 168. Quality zero through three uses
the retail gray, muted red, pale blue, and blue text colors. Category-four
definition zero takes the executable's shorter branch: Gold shows its stack
amount in the exact `Price                     :%9d` row, producing the wide
three-line panel instead of a name-only box.

The condition corner in `0x00465cb0` is reconstructed for backpack, equipped,
and pointer-held gear. Categories zero and one compare current and maximum
durability using the executable's integer percentage. Values below ten
percent draw `Status.njp` pattern 16 at
`(x + width*32 - 16, y + height*32 - 16)`. A nonzero value blinks for eight
game updates and hides for eight; zero durability stays visible. The adjacent
pattern-17 state has not yet been given a trustworthy gameplay owner.
Special-item script operations, consumable effects, dynamic dyed colors, and
network replication remain.

The in-game settings panel now follows `0x004103c0`. Escape opens the original
two-layer `Status.njp` panel and suspends world input. Boolean options use the
retail ON/OFF cells, click range uses its five 24-pixel cells, clicking a
priority class moves it to the right-hand end exactly as the executable does,
and both volume sliders use the original mute sentinel and `-3000` through
zero scale. Pointer, shadow, occluding-object, effect-volume, and BGM-volume
changes apply live; all changed configuration fields are written back through
the reconstructed 64-byte `SFlare.Cfg` writer when the game exits. The
screen-mode row is hidden and the LWL window stays windowed, but y=86 remains
empty so every later row keeps its retail coordinate. Mission and Map now
open their own screens from the original rows: Mission is modal, while Map
leaves the right-hand world viewport live.

The Help row and `H` shortcut now open the screen drawn by `0x0040e710`.
Status patterns 10 and 66 provide the authored 640-by-415 frame and the
230-by-128 action preview. Font01 text keeps the retail coordinates, colors,
shadows, row spacing, and original wording. The player uses CAF chart 7 at the
preview anchor. Help entered through Settings also runs the shared
`0x004088b0` `CLOSE` animation with Status patterns 27 through 30; Escape or
any click above the HUD dismisses it. Drawing the current owned companion in
the preview waits on the companion-ownership slice rather than borrowing a
town NPC.

The Mission List follows `0x0040cea0`. Status pattern 10 supplies the authored
640-by-415 frame, patterns 110 through 113 select either 24-entry page, and
patterns 25 and 26 provide the unfinished and cleared locks. Titles come from
`Table.Tbd` table 41; mission `n` gets its description lines from table
`700+n`. Script state zero hides a row, one draws the bright unfinished row,
and two draws the gray cleared row. The list keeps the retail 12 rows per
column, 27-pixel row cells, page-tab hit boxes, `Q` shortcut, and Settings
entry. Page-tab selection uses retail sample 58. Clicking a row opens the same
tinted patterns 59 and 58 detail panel with the bracketed title and 16-pixel
description lines. A click on the detail returns to the list, while Escape,
`Q`, or a click outside the list but above the HUD closes it.

The live half-width Map follows `0x0040d4d0`. It loads the current scenario's
`Scenario.Njp`, clips it to x=32..318 and y=40..374, and covers unexplored
territory with the same map-sized mask used by retail. Movement reveals the
retail 68-by-46 rectangle around each visited position. The player stays at
the original 160,210 map anchor while arrow keys scroll by 16 horizontal or
10 vertical pixels, Enter recenters, and the marker from `MapIcon.njp` uses
the 15-of-20 blink. Status patterns 71 and 118 provide the authored frame and
pulse. Its `0x11` UI mode leaves simulation and world input running in the
right half, moves the camera anchor from x=320 to x=480, and prevents Map-panel
clicks from leaking into the world. `N`, the Settings row, Escape, and a
secondary click above the HUD open or close it through the matching retail
paths. Loading and writing the per-save `Save/M%08d%02d.msk` history,
other online-player markers, and later scenario transitions remain with their
future owners.

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
its repeated player record, then parses the exact item prefix used by
`0x0044b580`: eleven optional equipment records followed by backpack, belt,
and special-item containers. The nine player equipment slots, 9-by-4
backpack, 4-by-2 belt, and 9-by-10 Special Item container now save and load
their category, definition, grid position, Gold quantity, durability, quality,
and category-sized instance state. The two still-unnamed equipment records
and all trailing payload bytes remain byte-for-byte unchanged. The loader also
skips the first counted flag array after the items and restores the following
51 transport flags against Table 40. Tests cover a new world save/load round
trip and unchanged re-encoding of an original retail save. Scenario, position,
mines, quests, and the remaining dynamic payload still need owners. Writes go
through a sibling temporary file
and protected replacement so a corrupt source or failed write does not
silently destroy the slot.

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
area title. At `0x324`, the first three counted lists preload object, PEOPLE,
and enemy resources. The object, PEOPLE, enemy, and item groups then use a
shared variable entity record before tails of `0x34`, `0x2c`, `0x13c`, and
`0x10` bytes. The portable decoder walks that sequence exactly, reads the
entry table and three-word footer in forward file order, and rejects truncated
groups or trailing bytes. All 209 shipped MCT files pass this path, covering
5,203 objects, 163 PEOPLE records, 18,788 enemies, and 84 placed items with no
mismatch between a nonnegative object, PEOPLE, or enemy resource and its
preload list. Enemy tails retain their fixed AI-controller name separately
from the surrounding parameter blocks.

The first enemy runtime follows the common half of that loader and
`0x00458f40`. Enemy character numbers use `14000000 + local ID`; the actor
keeps its MCT state, position, judgement, direction, name, optional part
table, resolved `Character\ENEMY` animation, and AI-control name. The
Wasteland of Pillars fixture creates all 66 records transactionally. The
constructor-selected action seven reaches `0x0045b600`, which draws CAF chart
zero and advances one frame per active-map update. Enemies join the ordinary
shadow/visible depth passes and their enabled judgement rectangles block the
player. The catalog also contains 34 invisible, non-colliding `Enemy Hole`
actors with resource `-1`; these remain live script/AI identities without a
fabricated visual. Live AI selection, movement actions, life changes, combat,
death, and drops are not inferred by this initial actor slice.

The loader rearrangement now has three more named consumers. Pre-AI MCT values
1 through 4 become the spawn-relative patrol rectangle read by `0x0045c3c0`.
Pre-AI value 8 is copied twice, initializing both current life at runtime
`+0xd4` and maximum life at `+0xe4`. Post-AI value 54 becomes the
thousandths-based movement speed scale at `+0x1dc`. `EnemyActor` owns those
typed values while preserving both complete raw blocks for later work.

The global AI catalog now has its portable owner too. `0x004127d0` loads
`Control.aid` before scenario actors are created; the preserved version-one
file contains 64 exact-name lists, 18 event buckets per list, and 1,338 action
candidates. `AiControlDatabase` retains the nine parameter and six condition
integers without speculative field names. All 18,788 MCT enemy records resolve
their controller name, and each live enemy stores the resulting stable list
index.

The evaluator at `0x0045c9f0` is now reproduced as an executable-owned,
deterministic unit. Condition values zero through two gate inclusive current
life percentage, values three through five request a target inside inclusive
distance limits, parameter zero supplies priority, and parameter two supplies
weight. Its temporary linked list reverses file order and retains a later
lower-priority candidate until another new maximum clears the list; the
portable path preserves that retail quirk. Events 1 through 10, 16, and 17
fall back to event zero when nothing is selected. Live attachment remains
pending until `0x00459500` target lookup, enemy life fields, selected-action
storage, and the native action dispatcher can be connected without a partial
behavior path.

The first native dispatcher pair is reconstructed separately. `0x0045c350`
implements action zero as a timed idle: AID parameter one is its duration,
event 11 holds it active, and event zero is restored when the counter reaches
that duration. `0x0045c3c0` implements action one as a patrol inside the four
spawn-relative MCT bounds. AID parameter three is multiplied by the MCT speed
scale and divided by 1,000; parameters four and five control moving and idle
updates, and parameter six chooses the walking chart. Event 12 holds the
action and event one ends it.

Mode three in `0x00454310` draws the patrol X and Y independently from
inclusive ranges on its first active update. A zero movement limit returns
before either draw. The portable controller pins all 61 shipped wait actions,
all 92 patrol actions, and the six zero-duration patrol cases, but stays
dormant while later presentation-side behavior remains incomplete.

The three short dispatch handlers after patrol are pinned too. `0x0045c560`
maps AI actions two through four onto presentation actions one through three,
and `0x0045c5a0` maps actions five through seven onto presentation actions
four through six. They clear the current presentation and reset the elapsed
action counter on entry, then do nothing on later updates while the
presentation routine owns completion. `0x0045c5e0` handles action eight by
resetting the counter on entry without changing the current presentation.

The shipped catalog uses action two 450 times, action three 158 times, action
five 178 times, action six 91 times, and action seven 42 times. It contains no
action-four or action-eight records. Their native handlers are still covered,
and unsupported actions nine through eleven remain unable to alter controller
state.

Remote Town's object group contains local IDs `0`, `200` through `204`, and
`300`, using `Character\OBJECT` resources 8, 15, and 14. Record 300 is named
`Warehouse`. `0x0045dd00` maps the 13-value tail into the type-zero runtime
object: its first three values select static-pattern or CAF-chart drawing,
while later confirmed values supply status bit `0x80`, height, draw flags and
strength, and RGB strengths. The remaining raw values are preserved without
guessed names. Remote Town's enemy and item counts are both zero. Entry key
zero supplies (`89898`, `2811`, direction `3`), the MCT map path selects
`f00_01`, and its footer is `{0, 0, 2000}`.

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

All seven records use the same type-one PEOPLE updater. Their final tail
values account for the visible differences: runtime offset `+0xd4` permits
native action 21 to face its evaluated target, while the preceding loader
conversion at `+0xd0` enables autonomous wandering only when the raw value is
zero. Malse disables both, so his opening action 21 intentionally leaves his
direction alone. Syria and the four animals can turn but do not wander;
Ostare enables both. Native action 18 only suspends movement and starts the
interaction, and action 19 releases it. The remaining tail value is `-65` for
all seven and is still unnamed.

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
dynamic depth pass. The first impact emits the retail selector-two item sound;
the second impact is silent. Unnamed definition fields remain preserved as
raw bytes.

The loader's separate MCT item branch sets initialization mode one. Those
actors use script character number `18000000 + local ID`, copy the three
common state channels, replace the common judgement with
`[-20,-20,19,19]`, and start at height zero in bounce state two. They
therefore do not run the drop arc or emit either impact sound. One MCT record
creates one actor; only category-four definition zero receives an inclusive
random quantity, without the script drop path's 10,000-Gold splitting.
Ground resources zero through six use the animated `Animation.*` layout,
while resources seven through eleven use static `Pattern.njp` and
`Pattern.sdw`; definition offset `0x34` is the CAF chart or direct pattern
index respectively.

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
PEOPLE walk at `0x0045da25` supplies dynamic collision mask `0xffffffff`.
`0x004145b0` excludes only the mover's own character number, so wandering
town actors also collide with the current hero and every other live PEOPLE
rectangle. The portable scene keeps that shared set current after each actor
update and has direct tests for self-exclusion and actor detours. The
159-unit rectangle range ends an interaction approach before that target
becomes a collision. Fixtures cross the sacks beside Ostare, cover the exact
Ostare-to-Malse route with live actors present, and use successive ordinary
movement legs for longer trips to companion interactions. The renderer
reads chart zero for idle and chart one for walking directly from player state,
rebuilds the depth key from the moving position, and follows the player's
projected position with the retail camera offset.

The dynamic-entity loop at `0x00429ce0` calls every active-map entry without a
camera or clip test. Its insertion path at `0x004298c0` orders entries by
character number. The gameplay frame updates the player before that scenario
loop. Type-zero objects use character numbers `10000000 + local ID`; PEOPLE
use `12000000 + local ID`. The portable scene therefore updates the hero, all
seven Remote Town objects, and then all seven PEOPLE records, including actors
outside the current view. Each later actor sees positions produced earlier in
the same update.

The common MCT vector previously labelled as part overrides is the initial
three-channel entity state. The loader at `0x004300e0` maps runtime offsets
`+0x4c`, `+0x50`, and `+0x54` to script keys based at 100, 300, and 200
million respectively. The type-zero paths at `0x0045ddd0` and `0x0045e080`
use them as visibility, pointer-selection, and judgement/collision flags.
Remote Town's exact object triplets are `{0,1,0}`, `{1,1,1}`, `{1,0,0}`,
`{0,0,0}`, `{1,0,0}`, `{1,0,0}`, and `{1,1,1}`. Its first three PEOPLE
records start `{1,1,1}` and all four companion records start `{0,0,0}`.

The type-zero object constructor at `0x0045dca0` is now represented by a
portable actor. Resources 8 and 14 supply static `Pattern.Njp` and
`Pattern.Sdw`; resource 15 supplies the paired `Animation.Caf` and
`Animation.Njp` path. Static and animated objects use the MCT pattern/chart,
height, status, strength, and RGB fields in the shared shadow and visible
display lists. Judgement-enabled objects join the live movement blocker set.
Pointer-enabled objects now use opaque static NJP or current CAF cells, the
shared range square and priority path, a +300 pale tint, and their MCT
nameplate.

Object 200 status zero emits opcode 37 at `0x004334da` and opens the transport
owner rendered by `0x0040c950`. It reads the 51 destination name/scenario/entry
triples from Table 40, compacts enabled rows into ten entries per page, uses
Status patterns 13, 22 through 24, 11, and 12, and plays sample 58. New
characters enable row zero (`Remote Town`, scenario 0, entry 50). Its
same-scenario selection resolves entry key 200 and relocates to
`(94685,-2756)`, direction 7. Warehouse status zero emits opcode 41 argument
zero at `0x004335ac` and toggles the existing Special Item owner.

The `0x00426200` transition signature is now separated from the transport UI.
Its scenario argument selects a `%08d` decimal directory. A nonnegative entry
value resolves MCT key `local-player-number + entry-value * 4`; `-1` uses the
explicit coordinate pointer instead. The same-scenario branch skips resource
teardown, while a changed scenario releases the old dynamic/map owners,
switches music, reads the new map and Scenario NJP, and then applies the same
entry rule.

The portable fresh-world path accepts scenario ID, entry value, and local
player explicitly. The first cross-map fixture is Table 40 row one: scenario
6, entry 4, key 16. It loads Wasteland of Pillars, `Map\f00_07.map`, position
`(35105,-6156)`, direction 7, BGM 1, 35 type-zero objects, and two PEOPLE
records. Its map-local MCT, SCS, collision, patterns, overview, exploration,
actors, and ground items now share one `ScenarioWorld` lifetime. Player data,
all four item owners, quests, missions, transports, and common resources live
outside that boundary. Script data is adopted only after the full scenario
owner succeeds, leaving the callback-bearing interpreter runtime in place.
Types 10 through 13 script values live outside the scenario transaction.

Live travel now uses the boundary: the same-scenario branch only relocates,
while a changed scenario prepares every new local owner before commit. A
failed load leaves the old map, script, player, items, missions, quests, and
transport flags usable. Success clears stale local interaction/audio state,
adopts the new SCS, relocates, changes BGM, and holds gameplay during the
standard `Waiting.njp` pattern 4 plus `WaitIcon.njp` 120-render-frame fade.
The icon uses x positions 590/598/606 in five-frame phases at y=440, matching
`0x00417bd0`. The alternate `VisualNN` selector remains pending. All 51
Table 40 rows are also checked against their shipped scenario directory and
single-player MCT entry.

Periodic status kind five executes independent scenario callbacks. Remote
Town sentences 158, 173, 188, and 203 read player-record offset `0x140`
through opcode 44, compare it with each companion type and the play mode, then
use opcodes 22 or 23 to set all three entity channels. In single player this
hides the player's own companion and enables the other three town dogs.
Sentence 149 is a separate distance/effect callback and still reaches
unsupported native behavior; it must not stop the later independent status
records from running.

Owned inventory interaction now follows `0x00445bd0`, `0x00446320`,
`0x00447290`, `0x00447970`, and `0x004087b0`. Backpack and special-item clicks
address their visible grids, the four accessory cells join the equipment
owner, and category-three objects use the separate staggered belt. An owned
item is removed from its container and carried by the shared item pointer,
while its inventory artwork is centered under the cursor using the complete
multi-cell footprint. Placement rounds that centered top-left corner to a grid
cell, rejects out-of-bounds and multi-item overlaps without losing the held
item, and leaves a single displaced item on the pointer. Closing either panel
does not discard the held item.

Clicking the live world with an item follows the branch at `0x00441d96`.
`0x00413ec0` chooses one of eight directions from the hero to the pointer and
the drop is placed exactly 200 world units away on that direction's axes. It
then re-enters the same ground-item resource, CAF, color, bounce, depth, hover,
and pickup path as scenario-created items.

This is still not complete gameplay. Enemy AI and combat, dynamic collision
for enemy movement, remaining script commands and operand domains, alternate
conversation modes, darkness, and saved-game scenario restoration are the
next executable layers.
