# ShadowFlare.exe Reverse Engineering

## Overview

The ShadowFlare.exe is a 532KB Win32 GUI application with ~1043 functions.
It was compiled January 14, 2003 and uses custom DLLs (RK_FUNCTION, RKC_*) for most heavy lifting.

Developer: **Missinglink**  
Publisher: **Denyusha**  
Window Class: `SHADOW_FLARE`  
Window Title: `ShadowFlare for Window98/Me/2000`  
Registry Key: `HKEY_LOCAL_MACHINE\SOFTWARE\Missinglink\ShadowFlare`

## DLL Import Summary

| DLL | Import Count | Purpose |
|-----|--------------|---------|
| RKC_RPGSCRN | 88 | RPG screen rendering, objects, maps |
| RKC_NETWORK | 38 | Multiplayer networking |
| RKC_RPG_SCRIPT | 25 | Scenario scripting |
| RKC_RPG_AICONTROL | 20 | AI/NPC behavior |
| RKC_DBFCONTROL | 20 | Double-buffer control |
| RKC_UPDIB | 17 | Sprite rendering |
| RKC_DIB | 14 | Bitmap/graphics |
| RKC_RPG_TABLE | 9 | Data tables |
| RKC_DSOUND | 9 | DirectSound audio |
| RKC_MEMORY | 6 | Memory management |
| RKC_FILE | 6 | File I/O |
| RKC_FONTMAKER | 4 | Font rendering |
| RKC_WINDOW | 2 | Window management |

Total: 258 RKC DLL imports

## Key Addresses

| Address    | Description |
|------------|-------------|
| 0x00401550 | CheckSingleInstance - Mutex check for single instance |
| 0x004014a0 | ProcessCommandLine - Parse command line args |
| 0x00401eb0 | LoadConfig - Load SFlare.Cfg configuration |
| 0x004016b0 | InitGame - Initialize all game subsystems |
| 0x004022e0 | WinMain - Main entry point |
| 0x00402520 | CreateGameWindow - Window class and creation |
| 0x00402b30 | WndProc - Window procedure (1142 bytes) |
| 0x00402840 | KeyDownHandler - Keyboard input |
| 0x004026d0 | KeyUpHandler - Key release (stub) |
| 0x004028a0 | LeftClickHandler - Left mouse button |
| 0x00402900 | RightClickHandler - Right/middle mouse |
| 0x00402920 | PaintInitialLoadingScreen - cached loading page and overlay |
| 0x004030F0 | RenderWorld - ground, shadow, and depth-sorted object passes |
| 0x0040CEA0 | RenderMissionList - two pages, script state, and detail panel |
| 0x004023d0 | UpdateGameState - Game state machine (switches on state 0/1/2) |
| 0x00401b90 | Shutdown - Cleanup all subsystems |
| 0x0041d970 | BootstrapGameplay - first-frame scenario setup (2936 bytes) |
| 0x00420df0 | LeaveTitleState |
| 0x00421bf0 | LeaveCharacterSelectState |
| 0x0041d880 | LeaveGameplayState |
| 0x00417bd0 | RenderLoadingScreen - gameplay loading presentation |
| 0x0046863c | CRT Entry - Runtime startup |

## Global Variables

The game uses separate globals, not one monolithic struct:

| Address    | Type | Description |
|------------|------|-------------|
| 0x00482770 | HINSTANCE | Application instance handle |
| 0x00482774 | LPARAM | Last lParam from WM_CREATE |
| 0x00482778 | SFWindow | Window object (~0xB100 bytes) |
| 0x00482D10 | RKC_UPDIB* | Sprite system |
| 0x00482D18 | RKC_DBFCONTROL* | Double buffer control |
| 0x00482D20 | RKC_RPGSCRN_CHARANIMBLOCK* | Character animation |
| 0x00482D24 | RKC_NETWORK* | Network system |
| 0x00482D28 | RKC_DSOUND* | Sound system |
| 0x00482DB0 | int | Game mode (0=SP, 1=Server, 2=Client) |
| 0x0048D71C | int | Screenshot requested flag |
| 0x0048D8B8 | int | Window style index |
| 0x0048D8CC | int | IME enabled flag |
| 0x0048D8D4 | int | Initial loading state (-1 hidden, 0/1 loading, 2 confirm, 3 accepted) |

## Game State Machine (0x004023d0)

The game uses a state machine with 3 states stored at SFWindow+0x59C:

| State | Handler   | Description |
|-------|-----------|-------------|
| 0     | 0x420df0  | Title screen / Main menu |
| 1     | 0x421bf0  | Character selection / save selection |
| 2     | 0x41d880  | Main gameplay |

State transitions:
- State 0→1: Open new-character or saved-game selection
- State 1→2: Confirm a single-player or network mode
- State 2→0: Return to menu

The visible loading screen is a sub-state of gameplay rather than top-level
state 1. At application startup, the game decodes `Waiting.njp` patterns 0, 1,
3, and 2 into cached DIBs. Initial single-player entry paints the Episode 1
background and loading label through `0x00402920`. Once scenario setup marks
the world ready, the label is replaced by a 16-pixel horizontally moving
arrow; Return or a click in its bottom-right rectangle continues into the
world.

`0x00417bd0` is not the ordinary map loading screen. It is a later
story/briefing visual presenter: it selects the Epilogue artwork from
`Waiting.njp` pattern 4 or an alternate `VisualNN.njp`, fades it over 120
rendered frames, and animates `WaitIcon.njp`. The owner of its nonzero visual
selector still needs to be tied down. The portable runtime no longer calls
this routine after every map change.

Retail's ordinary map transition remains black with the crossed-swords
`Waiting.njp` image and its `LOADING` plate only while loading is in progress.
The portable scenario transaction is currently synchronous and completes
between presented frames, so a fast transition goes directly to the new map
instead of manufacturing a fixed delay.

The initial scenario map is loaded by the large transition routine at
`0x00426200`. The `f00_01.Lst` indices are preserved across ground and object
records: NJP entries hold visible patterns and their following SDW entries hold
one-bit shadows. `0x004030f0` builds separate shadow and visible-object lists,
inserts both static scenery and the current dynamic actor block, then calls
`RKC_RPGSCRN_OBJECTDISP::SortDisplayObject` on each combined list.

Insertion first groups status classes and compares the projected
`position + (judgement.left, judgement.top)` point. The final sort cannot be
replaced by one Y key: it compares the absolute left, top, right, and bottom
edges of every remaining judgement rectangle. A candidate is held back when
another rectangle begins before its right and bottom edges and ends before at
least one of them; that other entry must be drawn first. All comparisons are
strict, so merely touching edges do not create a dependency. This is what puts
a character behind Remote Town's long walls and houses even when the visible
sprite covers a large screen area.
Status bits `0x100`, `0x80`, and `0x20` select classes one, two, and three;
class zero is the default. The packet sequence draws non-default shadows and
objects first, followed by default shadows and objects.

The portable `RKC_RPGSCRN` library now owns that exact ordering rule. The
renderer supplies decoded OBL bounds and the player/NPC judgement rectangles,
keeping DLL-derived behavior out of the executable-owned render code.

The MCT loader at `0x00427b50` first reads a 16-byte
`MCED DATA v0000\x1a` signature, two 260-byte paths, two unknown 32-bit
values, the music index, and a 256-byte title. Its variable entity section is
followed near EOF by a count and 16-byte entry records in key, world X, world
Y, direction order. `0x00427930` searches those records by key. The portable
loader now uses scenario `00000000`'s map path and entry key zero rather than
embedding `f00_01`, (`89898`, `2811`), direction 3, and music index 0 in
`WorldScene`.

The item group near the end of that variable section constructs ordinary item
actors with initialization mode one and script identity
`18000000 + local ID`. `0x00458930` immediately turns that mode into the
settled state: zero height and vertical speed, bounce state two, runtime
judgement `[-20,-20,19,19]`, and no landing sound. Definition offsets `0x30`
and `0x34` select the `Character/ITEM` resource and either a CAF chart or, for
the static resource layout, a direct `Pattern.njp`/`Pattern.sdw` index.
Category-four definition-zero records are the sole branch which rolls an
inclusive MCT quantity; one source record still creates one actor.

The enemy branch creates character number `14000000 + local ID`, resolves the
fixed 32-byte controller name through `RKC_RPG_AICONTROL`, and rearranges the
15 values before that name plus the 56 values after it into a 72-word
initializer consumed by `0x00458f40`. That function copies the complete block
to runtime offset `+0xc4`, synchronizes the screen object, and initializes the
AI-list slot to `-1`. The default action selected by the constructor is action
seven; `0x0045b600` renders CAF chart zero using the MCT direction and advances
its frame counter on every active-map update.

The first four patrol bounds consumed by native AI action one come from
pre-controller values 1 through 4 and are added to the enemy's spawn
position. Pre-controller value 8 is intentionally duplicated by the loader:
it initializes both current life at `+0xd4` and maximum life at `+0xe4`.
Post-controller value 54 becomes the movement-speed scale at `+0x1dc`.
Movement actions calculate `AID parameter 3 * scale / 1000`, using integer
arithmetic before passing the result to the shared movement controller.

`0x004127d0` loads the global `System/Game/Parameter/Control.aid` catalog
before scenarios create enemies. Its version-one file contains 64 named lists,
18 event buckets per list, and 1,338 action records. The scenario loader uses
the DLL's exact-name lookup and stores the returned zero-based list number in
the enemy initializer. The portable `AiControlDatabase` follows that ownership:
it is loaded once by `WorldScene`, while scenario-local enemies retain a
non-owning list reference and its stable index.

Enemy update reaches the event selector at `0x0045c9f0`. Parameter zero is
the candidate priority and parameter two is its random-selection weight.
Condition zero enables a current-life percentage test with the inclusive
minimum and maximum in conditions one and two; condition three enables the
same kind of inclusive range around target distance using conditions four and
five. A bound of `-1` is open. The target query at `0x00459500` searches the
four player slots and then companion character numbers `16000000` through
`16000003`. A player is eligible only when its active state is exactly one,
it belongs to the enemy's scenario, and—when requested—its life is positive.
The nearest eligible player wins, with the lower slot winning a tie. The
companion pass only runs when no player qualified. It requires the actor's
script-active bit, positive life when requested, and zero in the owning
player's companion-mode field. Companions use the same strict-nearer tie rule.
All distance tests use the two judgement rectangles, not actor origins.

The default lookup at `0x004593f0` is deliberately separate. It still prefers
any eligible player over every companion, but accepts any nonzero player
active state and searches all type-five companion actors in scenario order.
Its companion path does not repeat the script-active-bit test, though it still
requires owner mode zero and optional positive life. The portable enemy target
selector keeps both entry points behind one typed query result so the event
evaluator and native movement/presentation actions cannot disagree about
target kind, identifier, measured distance, or the position used for facing.

The scenario loader's rearrangement is important for the six non-movement
presentations. Direct actions one through three read maximum target distance
from post-controller values 3 through 5, animation chart offsets from values
41 through 43, and speed-table indices from values 47 through 49. Retail adds
four to those authored chart offsets. Effect actions four through six use
post-controller values 9 through 11 as effect types, 15 through 17 as
subtypes, 12 through 14 as their direct parameters, and 18 through 20 as
additive values. Their chart offsets come from values 44 through 46 with a
retail base of seven, and values 50 through 52 select animation speed.
`0x00458f40` sees these fields at runtime offsets `+0x110` through `+0x170`;
the portable profile exposes the proven consumers while retaining both raw
MCT blocks.

`0x00459290` dispatches presentation actions one through three to
`0x0045a2f0` and actions four through six to `0x0045ac90`. On entry, the
first family searches from distance zero through its authored maximum; the
second uses the default target query. Either one faces the selected actor,
starts at frame zero, and keeps that target for the presentation-side result.
The ten animation multipliers are exactly `0.3`, `0.4`, `0.6`, `0.8`, `1.0`,
`1.5`, `2.0`, `2.5`, `3.0`, and `4.0`. A continuing update multiplies its
integer elapsed counter by the chosen value and truncates toward zero.

Both routines scan part zero for every newly crossed CAF frame. Status bit
`0x40` is the impact point; bits `0x400`, `0x800`, and `0x1000` select the
three sound-marker slots consumed by `0x0045a2a0`. A frame which jumps beyond
the end is not scanned before the draw frame is clamped, an odd but observable
retail edge case. Reaching the last frame draws it once, restores presentation
seven, and publishes events two through four for the direct family or five
through seven for the effect family only when the existing event is `-1`;
another event is never overwritten. A resource-less actor completes
immediately. The portable controller emits typed impact/effect and audio
marker results; damage resolution and effect construction remain separate
consumers rather than being hidden inside animation timing.

The sound helper first indexes `DAT_00480a20` as
`resource * 30 + marker * 10 + chart`. It is exactly 25 enemy resources by
three marker slots by ten charts, with 59 non-`-1` overrides. When that cell
is `-1`, `DAT_004809a8` supplies a marker-by-chart fallback. Only chart three
has one: sample 86 for every slot. The portable resolver preserves that
override-first order and hands the resolved sample back with the presentation
update, leaving actual playback to the world audio owner.

Damage itself does not belong to either presentation routine.
`0x00412a40` is a shared packet-to-damage function called by the player
receiver at `0x00443cb0` and the enemy receiver at `0x00459690`. Its first
argument is the same 77-word packet built by direct attacks and effects. Its
second argument is a 14-word receiver profile. Word zero selects the receiver
formula family, words three and four are the two defense values consumed by
ordinary and effect paths, words five onward provide the table-indexed
elemental modifiers, and word 13 is the receiver's native element.

Packet word 37 equal to one is an immediate override: word four is returned
unchanged, even when it is below one. Every calculated branch clamps to one.
Effect-family packet kind three uses table 11 with receiver family zero, or an
element comparison and receiver word four with families one and two. The
ordinary dispatch combines the low 16 bits of packet word zero and receiver
word zero. Its four accepted pairs use table 7, table 11, or the shared
elemental calculation; every other pair returns one without consuming
random state.

The elemental calculation compares packet word 32 with the receiver's native
element and its opposing pair: Fire/Water, Earth/Thunder, Holy/Dark, and
Gel/Metal. The opposing element gets `rand() % 4 + 10`, the same element gets
`rand() % 4 + 7`, and any other element gets ten without that optional draw.
A second `rand() % 3 + 9` supplies the base multiplier. The table-7 branch
instead draws its base factor first, asks the character owner to resolve
packet word two, and indexes the packet through the opposing element.
The source lookup does not change the arithmetic, but it is retained as a
typed request so the later live receiver can preserve the call.

The portable resolver keeps these lookups and draws in retail order. A missing
table or unsafe index is reported as invalid instead of following the
executable's unchecked pointer behavior. Receiver-owned barriers, life and
mana changes, reaction state, equipment durability, status application,
reflection, death, and drops stay outside this shared arithmetic boundary.

The enemy receiver at `0x00459690` is now reconstructed on top of that
boundary. The 72-word enemy initializer supplies native element, physical and
magical defense, hit-reaction chance and duration defense, and the flag that
always suppresses reaction displacement. Those values come from MCT pre-AI
words 6, 9, and 11 and post-AI words 38 through 40. They are decoded once
with the enemy rather than looked up again by the receiver.

The callback rejects an already defeated enemy and a negative source before
doing anything. A non-positive packet base skips the shared damage formula
but still reaches the later status, effect, and event paths. Damage is applied
only when the packet's source slot matches the local player slot. In
single-player and server mode, the amount credited to that slot is capped by
the enemy's life before subtraction. A surviving hit asks the server path to
broadcast its seven-field damage record. Client mode instead sends that record
to the server and keeps its predicted local enemy at a minimum of one life.
This branch is also the evidence that global mode one is server and mode two
is client.

Visual enemies calculate hit reaction before life changes. Table 25 selects
chance and duration from damage as a percentage of half maximum life, capped
at row 49. Table 24 adds the packet's authored affinity grade for player
attacks or row five for an opposing-element non-player packet. Effect-family
packets replace chance, duration, and displacement suppression from their three
element-indexed packet banks. The chance comparison consumes one draw even
when its final value is zero. A reaction without movement is capped at 15
updates before packet word 76 is added; an enemy's always-suppress flag is
applied after that cap. This ordering is intentional. The names matter here:
a zero in the stored motion field permits the later action-ten impulse, while
a one prevents it.

A started reaction selects presentation ten, records the impact angle, faces
the source when motion is disabled, locks the action, and preserves packet
reaction stages one and two. Stage one spawns effect 21015 through 21018 and
plays sample 119. Reflection word 39 emits five effect-20013 requests with
five independent spread draws and sample 61. Packet words 34/35 and 74/75
emit their two configured effects with null combat-packet pointers. Word 72
uses a strict 20-percent draw followed by separate packet-kind and
21011/21012 draws. The portable effect request now records whether the retail
constructor received a packet pointer, so these paths cannot be confused with
the packet-owning attack effects.

A lethal authoritative hit records whether the packet was effect-family,
retains its full source character number, requests the retail kill-accounting
path, and emits the local player's enabled on-kill statuses 7, 8, and 9.
Presentation eleven and the action lock are selected only after reflection
and packet effects have had their normal chance to run. Player direct hits
now commit the returned life, attribution, reaction, event, and defeat state
to the live enemy and forward its audio requests.

The live presentation consumer follows `0x0045bb20` for action ten. CAF chart
two is sampled across the authored reaction duration: the displayed frame is
`counter * frame_count / duration`, with the last update forced to the last
frame. Sound-status lookup deliberately uses `counter % frame_count` instead
of that displayed frame. When displacement is not suppressed and packet word
76's delay has elapsed, each update projects the enemy along the stored impact
angle by `(duration - counter) * 120 / duration` and passes the segment through
the ordinary map, object, and actor collision owner. Completion unlocks the
actor, restores presentation seven, and publishes event 16 only from the
native minus-one event state.

Action eleven follows `0x0045bec0`. It selects chart three, switches to
direction eight only when that chart contains frames there, plays the separate
25-entry `DAT_004815d8` death sample on update one, and emits effect 21010
with one `rand() % 8` direction draw on entry. Once the CAF reaches its last
frame, the actor fades from strength 1,000 over 120 updates and is removed.
The chart-three marker fallback still supplies sample 86.

Receiver visuals now cross a world-owned effect boundary as well.
`0x00429ec0` maps effects 21000 through 21014 to the exact OPTION resources
11000000, 11000001, 11000002, 11000009, and 11000017 through 11000027.
`0x0042b860` creates the ordinary one-pass CAF owner at the source actor's
resolved position. The specialized `0x0042cba0` path used by 21010 through
21012 instead lasts 120 updates at initial strength 500 and fades during its
last 30. These visual actors participate in normal world depth sorting and do
not own damage. The four 21000 through 21003 variants are the ordinary
splatter selected by direct hit packets and play for both surviving and lethal
hits. Enemy death separately creates effect 21010. CAF frames advance once per
game update: that fixed-lifetime death effect reaches its last authored frame,
holds there, and only fades during the final 30 updates. Reflection,
staged-reaction, projectile, and spell effect
dispatchers remain separate follow-up branches. Network transport, experience
accounting, and drops remain outside this receiver/presentation boundary.

Eligible actions are copied into a temporary linked list at position zero.
Finding a priority above the current maximum clears that list first, but a
later lower-priority action is still inserted. Weighted traversal is therefore
in reverse file order and may include those later lower-priority candidates.
This is observable data behavior: 33 priority decreases exist inside the
shipped event buckets. If no action is chosen, requested events 1 through 10,
16, and 17 retry event zero. The portable event evaluator preserves this path
as a deterministic unit. Its target-condition callback now consumes the same
typed result as the action dispatcher. Live enemy updates remain off until
selected-action storage and the complete movement/presentation consumers can
be connected without a partial behavior path.

The native dispatcher at `0x00459340` sends selected action zero to
`0x0045c350` and action one to `0x0045c3c0`. Action zero resets its elapsed
counter on entry, requests presentation action seven when necessary, uses AID
parameter one as an inclusive duration, and exposes event 11 until it returns
to event zero. Action one builds an absolute patrol rectangle by adding the
four MCT offsets to the spawn position. It starts movement-controller mode
three for AID parameter-four updates, waits parameter-five updates, and repeats
until parameter-one total updates have elapsed. Its holding event is 12 and
its completion event is one.

Movement-controller mode three at `0x00454310` chooses each X and Y coordinate
with a separate `rand() % inclusive_size` draw on its first active update.
With a zero update limit it exits before either draw. The passive portable
controller reproduces this event, counter, presentation, speed, and random
contract without yet changing a live enemy.

The next dispatcher group is intentionally small. Actions two through four
enter `0x0045c560` and request presentation actions one through three; actions
five through seven enter `0x0045c5a0` and request presentation actions four
through six. Both handlers clear the current presentation and reset the
action counter only on entry. Their presentation routines later restore idle
action seven and publish the matching event number when the animation ends.
Action eight at `0x0045c5e0` only records its own entry and resets the counter,
leaving the current presentation alone.

`Control.aid` contains 450 action-two records, 158 action-three records, 178
action-five records, 91 action-six records, and 42 action-seven records. It
contains no action-four or action-eight records, even though both native
paths exist. These dispatch transitions and their animation, targeting,
marker, typed-effect, and completion-event consumer are portable and tested.
They remain behind the live-enemy boundary until damage/effect construction,
audio playback, and movement can be connected as one complete update path.

Actions nine and ten at `0x0045c600` and `0x0045c780` are the target-movement
pair. If the selected AID condition enables a target range, they repeat the
retail target lookup with its minimum and maximum; otherwise they use the
enemy's default eligible target. A missing target immediately returns event
nine or ten. A valid target starts presentation action eight at parameter
three times the MCT speed scale divided by 1,000. Parameter seven controls
target refresh cadence and parameter eight is the random-turn chance.

Action nine retreats until the bounds distance reaches 10,000, using movement
mode five for a player and mode two for a scenario actor. Event 14 holds it
until parameter one's inclusive duration, after which it returns event nine.
Action ten approaches to bounds contact with modes four and one respectively;
event 15 holds it before event ten. Either action also returns its completion
event if the walking presentation stops.

Action eleven at `0x0045c900` starts fixed-point movement mode zero toward the
enemy's cached walk point. Its speed is the AI list's `WalkPointSpeed` times
the MCT scale divided by 1,000, and its stop distance is 150. It keeps event
minus one through elapsed counter 90 and returns event zero at counter 91, or
as soon as the walking presentation stops. Retail `Control.aid` contains 61
action-nine and 205 action-ten records, but no action-eleven records. Of the
retreat records, 44 repeat an authored range lookup and 17 use the default
target. All approach records use range lookup; 30 enable the optional
random-turn and refresh pair.

Gameplay pointer selection starts at `0x0040ede0`, which asks `0x004165d0` to
collect the current display objects inside the configured pointer square. A
candidate must have an opaque pixel from a visible NJP part in that inclusive
square; this is not a loose actor-bounds test. Turning the range option off
reduces the test to the exact cursor tip. Within one priority group, an exact
tip hit wins first and the nearest candidate wins otherwise. The candidates
also keep their normal world depth order and are grouped by the five
priorities stored in `SFlare.Cfg`. With the retail defaults, a ground item
wins over a person when both are in range, while type-two enemies use the
highest `ENEM` priority and win over both.

`0x0040ee70` scans that result from front to back. For a person it projects the
actor's feet, subtracts the MCT label height, draws a half-transparent black
plate around the centered 6-by-12 name, then draws a black one-pixel shadow
and the actor's configured name color. Ground items use the name from
`Item.Ibn`; ordinary money is formatted as its quantity followed by `Gold`.
The selected object's visible RGB strengths each receive `+300`. Values above
1000 do not multiply the palette color: RKC_UPDIB moves each channel toward
white, which produces the pale hover tint seen in the retail game.
Enemies use that same nameplate and pale-tint path, with their MCT name color
and label height.

Message layout at `0x00456550` counts ASCII and Shift-JIS glyph widths, adds
an eight-pixel text-box inset, and positions actor messages above the same MCT
label anchor. `0x00456bb0` surrounds that box with the nine-pixel
`Hukidasi.njp` frame and draws its tail from
`System\Game\Pattern\Hukidasi.njp`, then places the 6-by-12 `Font01.njp` text
at a four-pixel inset. The tail overlaps the bottom four frame pixels before
extending into the world. The portable renderer follows this path for actor
messages instead of using a fixed screen-bottom dialogue box. Message events
retain their current script character, which also anchors Syria's initial
branch even though it does not run an explicit facing command.

The same layout function gives `~` a special meaning when message flag
`0x40000000` is set. The tildes are removed from the displayed text and each
enclosed run is stored as a clickable character range with its line and
columns. The companion messages use opcode-2 mode one, pass a writable script
operand for the selected zero-based range, and supply an initial selection.
Dune's first menu starts on range three, `QUIT`. These bytes are UI control
markup, not part of the English message.

Mode one also carries companion follow-up text with initial range `-1`.
Those messages have no `~` spans and are acknowledged like ordinary speech;
they do not write a choice result. Harley's `Explanation` branch uses this
form for messages `1000057` and `1000058`, then reaches the same status-one
release chain. A non-negative initial range is therefore part of the choice
contract, not just a visual default.

Pointer handling at `0x00457fa0` replaces the current range only when
`0x00457bb0` finds the pointer inside one of those spans. Moving away leaves
the last range selected. `0x00456bb0` draws that range in red
`(255, 0, 0)` and the other ranges in gray `(96, 96, 96)`. The portable
conversation state now owns the same selection, so hover rendering and the
option returned to the interpreter use one set of range indices.

World interaction goes through `0x00449240`. It measures the shortest gap
between the player and target judgement rectangles with `0x004143c0`, rather
than comparing their center points. The player's initial interaction distance
at offset `0x3f4` is `0x9f` (159 world units). A target outside that distance
starts movement-controller mode one and is followed as it moves. Once the
rectangle gap reaches 159, the player faces the actor and starts that actor's
status-zero script. This is why clicking a distant person in retail walks
toward them instead of opening a remote conversation or simply rejecting the
click. Type-three ground items use the same approach path. Once close enough,
`0x004526a0` calls the local player's inventory insertion routine and removes
the scenario entity only when ownership has transferred successfully.

Type-two enemies split from the script-interaction path. A living, visible,
pointer-enabled enemy outside the same inclusive `0x9f` rectangle-gap range
becomes movement-controller mode one's moving target. On reaching the range,
the player stops, faces the enemy, stores its character number, and requests
the currently selected ordinary attack. The `SFlare.Cfg` field mirrored at
`0x0048d540` is the `Attack While Moving` gate used by this branch; retail
unconditionally restores it to enabled while loading the config. The portable
target controller now owns the approach-to-ready transition and cancels it if
the enemy disappears, dies, becomes hidden, or loses pointer status. CAF
attack startup and the impact marker are owned separately from that approach.

`0x00450630` chooses the ordinary player action from the equipped main-hand
instance's subtype. No main hand, or a subtype outside the five explicit
branches, selects action 7. Subtype 0 selects action 8, subtype 3 selects
action 9, subtype 1 selects action 10, and subtypes 4 and 5 select the
separate actions 19 and 20. The portable ordinary-action checkpoint implements
7 through 10 and recognizes 19/20 without pretending they use a melee chart.

Action 7 at `0x00439140` and actions 8 through 10 at `0x00435e60` share the
same authored timing contract. Action 7 and action 8 use CAF charts 5 then 6;
action 9 uses 15 then 16; action 10 uses 19 then 20. On entry the actor locks
movement, clears the displayed frame, sets the previously scanned frame to
minus one, and synchronizes the action. Action 7 calculates the displayed
frame before incrementing its counter, while 8 through 10 increment first.
This one-update distinction is preserved. Action 7 requests its swing sound at
counter five; 8 through 10 do so at counter six. A null or light main hand
uses sample 1, and weapon weight 60 or greater uses sample 2 through
`0x00466110` selector four.

`0x00450c60` derives the animation tier from Table 4 after capping the derived
Speed of Attack value at 255. The ten frame factors are exactly 0.6, 0.7, 0.8,
0.9, 1.0, 1.1, 1.2, 1.3, 1.4, and 1.5. Equipped weight greater than the
derived capacity forces tier zero. The action calls this calculation on every
update, so an equipment or weight change can affect an attack already in
progress. `0x00445630` counts equipped objects only
and omits the off hand when the main-hand classifier suppresses it; portable
weight and base-contribution accumulation now do the same.

The damage moment is not inferred from a frame count. The action scans part
zero of every newly crossed frame in its first chart and emits an impact when
CAF status bit `0x40` appears. It only scans while the global displayed frame
is still in that first chart, then subtracts the first chart's frame count,
clamps within the recovery chart, and unlocks on its final frame. The shipped
male and female chart-5 data both contain ten first frames with the marker at
frame 7 and nine chart-6 recovery frames. The portable world validates the
retained target again at the marker before publishing the typed impact event;
the next combat checkpoint owns packet construction and enemy mutation.

The portable first pickup checkpoint keeps that separation. `WorldScene`
owns the stable ground entity and pending approach, while `PlayerInventory`
owns the accepted category, definition ID, and quantity. Gold fills existing
stacks to 10,000 as `0x00449ef0` does. The retail 9-by-4 placement grid,
multi-cell item sizes, full-inventory failure, and inventory panel are still
part of the next inventory slice.

Inventory artwork goes through `0x00465cb0`, separately from the world item
CAF path. After drawing an ordinary weapon or armor icon, it divides current
durability times 100 by maximum durability. Results from zero through nine
draw `Status.njp` pattern 16 at the lower-right of the complete multi-cell
footprint. Nonzero durability uses the low four bits of the gameplay update
counter for an eight-update-visible, eight-update-hidden cycle. Zero
durability skips that blink and leaves the warning visible. Backpack,
equipment, and pointer-held items all reach the same drawing rule. The nearby
pattern-17 branch depends on a different runtime item state and remains
unclassified rather than being folded into the condition model.

`0x00454210` initializes the executable's shared movement controller and
`0x00454930` advances it. It is not an A* route search. A direct collision
sweep is followed by stateful cardinal obstacle-edge steering. Controller
modes cover fixed points, scenario actors, other players, bounded wandering,
and related approach behavior. Calls from the player, PEOPLE actors, and enemy
actors all reach this controller. `RKC_RPG_AICONTROL` chooses enemy intent and
parameters; it does not contain a second enemy pathfinder.

The portable `MovementController` now keeps the same pair of cardinal
movement and wall directions between updates. `0x00414990` sweeps along the
dominant axis, interpolates the other coordinate with integer arithmetic, and
returns the last free point before contact. During cardinal edge movement it
also checks a one-pixel strip on the wall side; that small side step is what
lets an actor stay against an edge and turn its corner without an axis-slide
shortcut.

On the first blocked direct step, `0x00454930` compares the attempted quadrant
with the returned contact. If no movement was possible, it probes east, south,
west, or north in the quadrant-specific retail order. Those results select
the cardinal movement direction and the side occupied by the wall. A blocked
edge step swaps them by taking the opposite wall direction. Leaving an edge
is controlled by the signs of the remaining X/Y distance and the exact
movement/wall pair. It is not based on whether the Euclidean distance happened
to improve.

The portable implementation follows those tables directly and has no A*
fallback. Live town actors take part in the same judgement query. The selected
interaction actor remains solid; mode-one interaction ends at the 159-unit
rectangle gap before collision. Player and PEOPLE movement share this owner,
including facing during detours. The PEOPLE walk at `0x0045da25` passes
dynamic collision mask `0xffffffff`, just like the player. Its own character
number is the only live entry excluded by `0x004145b0`. The portable world
therefore keeps one live blocker set for the hero and all PEOPLE actors,
excludes only the actor currently moving, and refreshes positions as actors
are updated.

Remote Town fixtures cover the sacks beside Ostare, the Ostare-to-Malse
approach with the town actors present, longer companion trips made from
successive ordinary movement commands, and a wandering PEOPLE actor steering
around another live actor. Per-actor retail type masks, enemy integration,
and the remaining controller modes are still follow-up work.

The scenario entity manager updates every active-map entry through its virtual
update slot at `0x00429ce0`; it does not first test the camera or render clip.
`0x004298c0` inserts those entries in ascending character-number order. In the
gameplay frame at `0x0041e4f0`, the player update runs before the scenario
manager at `0x004274f0`. Portable Remote Town follows that player-first order,
then updates its seven PEOPLE records in their MCT order, which is ascending
by local ID. This preserves both offscreen wandering and the live blocker
positions seen by the next actor in the same update.

The controller's cardinal values are `1 = north`, `2 = south`, `3 = west`,
and `4 = east`. The initial one-pixel probes are ordered by the attempted
direction:

| Attempt | Probe order if contact equals the start | Selected movement / wall |
|---|---|---|
| southeast | east, south, west, north | east/south, south/east, west/south, north/east |
| northeast | east, north, west, south | east/north, north/east, west/north, south/east |
| southwest | west, south, east, north | west/south, south/west, east/south, north/west |
| northwest | west, north, east, south | west/north, north/west, east/north, south/west |
| south or north | east, then west | east/vertical, then west/vertical |
| east or west | south, then north | south/horizontal, then north/horizontal |

The edge-stop tests are equally specific. For a destination northwest of the
actor, the stop pairs are south/east and east/south. Northeast uses
south/west and west/south. Southwest uses north/east and east/north.
Southeast uses north/west and west/north. On a shared axis, movement directly
away from the destination stops the edge walk. These pairs explain why a
generic “distance got smaller” test produces loops that the retail code does
not.

Player movement calls `0x00454930` with dynamic collision enabled and mask
`0xffffffff`; the resolver uses its low byte. `0x004145b0` excludes only the
moving character number and interprets the mask like this:

| Mask bit | Dynamic character class |
|---|---|
| `0x01` | local character numbers 0 through 3 |
| `0x02` | type 0 |
| `0x04` | type 1 |
| `0x08` | type 2 |
| `0x10` | type 4 |
| `0x20` | type 3 |
| `0x40` | type 5 |

That means the player query treats every relevant live actor as solid. It does
not remove the NPC selected for interaction. The controller's mode-one range
test is what prevents the player from trying to occupy that NPC's rectangle.
The portable player currently supplies all loaded town people to the same
query; preserving the individual class masks matters once enemies and other
players join the portable world.

## Player record and new-game tables

The character-selection object owns a 0x160-byte record beginning eight bytes
into its small wrapper. `0x00440f70` copies all `0x58` dwords into the gameplay
player at offset `+0x10`, then initializes the new-game defaults. The confirmed
record fields are name at `0x00`, gender at `0x18`, the save-menu job value at
`0x1c`, and level at `0x24`.

New characters obtain thirteen values from `Table.Tbd`. The selected table is
`0x385 - gender`. The saved field uses zero for female and one for male, so
this selects table 901 for female and table 900 for male. The values
are stored in a slightly shuffled part of the record because current life and
current mana sit beside their base maxima. Runtime `+0x40/+0x44` are base
maximum and current life, while `+0x48/+0x4c` are base maximum and current
mana. Both current values start at their maximum. `0x0044ea60` later builds
the separate derived values which include equipment and status modifiers.

The item panel keeps equipment separate from the backpack. Pointer classifier
`0x00447290` returns main hand for x=480 through 543 and y=16 through 143.
`0x00446320` accepts category-zero items there, compares the definition's
required-level field with player offset `+0x34`, and stores the equipped
instance at player offset `+0x4f4`. Clicking an occupied main hand with an
empty pointer unequips it; carrying another valid weapon swaps the two.

The decoded weapon record maps requirement level from field-block offset
`0x94`, player CAF part from `0xa8`, and that part's RGB strengths from
`0xac` through `0xb4`. Short Sword requires level one and selects part 12.
Its first two entries in the ten-value derived contribution block are 20 and
100. Retail runs both the derived-value refresh at `0x00450080` and appearance
refresh at `0x00444ca0` after changing equipment. The portable equipment owner
keeps those contributions available for combat, updates the visible 30 weight
immediately, and rebuilds the player part mask without making the item panel
own world rendering state.

The player receiver at `0x00443cb0` builds its 14-word defense profile just
before calling the shared damage routine. Word zero is the player family,
word one is the character number, words two through four are the already
derived attack, physical defense, and magical defense, and words five through
twelve are Fire, Water, Earth, Thunder, Holy, Dark, Gel, and Metal. Retail
does not write word 13 for this family, and none of its family-zero damage
branches read it. The portable snapshot clears it rather than carrying an
uninitialized stack value.

`0x0044fba0` derives the eight base affinities from a two-dimensional player
value. Its anchors are `(0,20000)`, `(0,-20000)`, `(-20000,0)`, `(20000,0)`,
`(14140,-14140)`, `(-14140,14140)`, `(-14140,-14140)`, and
`(14140,14140)`. Each result is
`(20000 - trunc(distance)) / 2000`, with signed division toward zero.
`0x0044fca0` then adds item contributions with 32-bit wrapping and clamps each
final value to `-10..10`.

`0x0044fe30` supplies those item values from the main hand, helmet, body,
boots, optional off hand, and four accessories. Weapons and armor combine the
definition's eight base values with instance words 39 through 46. Accessories
use only those rolled instance words. An identified category-two backpack
item also contributes when its inventory width is not one; ordinary one-cell
accessories in the backpack do not. A main-hand weapon classified as
two-handed suppresses the off hand here as well as in player drawing.
`0x004672f0` proves that classifier uses weapon subtype one or three and the
weapon field at raw offset `0xcc`; the previously suspected field at `0xdc`
is unrelated.

The third value in each serialized item header and runtime offset `+0x1c` is
the identified flag, not an item-quality tier. The same flag is mirrored in
instance word 48 for weapons and armor and word 47 for accessories. New
variant-one and variant-two definitions begin unidentified; the other
variants begin identified. Tooltip color comes from the definition's variant,
so loading an identified ordinary item cannot accidentally recolor its name.

The rest of `0x00443cb0` is now a passive portable receiver rather than an
early live-world shortcut. Increased Power multiplies physical defense by
`12/10` and adds two to spell level before the normal `1..20` clamp, global
magic-level addition, and final `1..30` clamp. A local Energy Shield uses
table 17 row 9 to scale ordinary physical defense and sends ordinary damage
to mana while mana remains. Effect-family damage still reaches life.

Magic Shield only handles effect-family packets for the locally owned player.
Table 17 row 18 supplies its reduction and a zero result is forced back to one
damage. Damage of at least 20 requests spell training. The shield emits effect
21029 and sample 60, then charges table 16 at the currently selected magic
row and Magic Shield's effective level. Equipped instance parameter 19 lowers
that cost but cannot take it below one. Empty mana disables the shield.
Counter Burst repeats that cost path with spell 19 after a valid reflection.

Lethal local damage searches the Special Item owner for category four,
definition `98000000`. The first match is consumed, both life and mana return
to maximum, sample 17 plays, and effect 21020 is requested. Without it the
player enters presentation action five. Retail then checks helmet, body,
off hand, and boots for one durability point with 20, 30, 30, and 20 percent
chances. Only occupied slots consume draws. The off-hand draw happens before
the two-handed-main-hand test, and a break requests both equipment sync and a
derived-value refresh.

Packet word 38 enables player reflection. The equipment pass always consumes
its chance draw and sums instance parameters 20 and 21 across active gear;
Counter Burst adds table 17 row 19. Only a live type-two scenario source can
receive the reflected immediate packet. It carries the player number, derived
physical defense, level, a random effect number from 20015 through 20017, and
at least one reflected damage. Source value 100 halves that damage. Equipment
reflection creates effect 20014 toward the source, while Counter Burst creates
21030; both play sample 60.

Player hit reaction uses the packet element to select one of the eight defense
affinities. Negative values `-10..-1` add table 26 chance and duration.
Table 25 supplies damage-scaled defaults, while effect packets select their
negative, neutral, or positive chance, duration, and motion banks. Instance
parameters 14 and 15 defend chance and duration, and parameter 16 forces
motion after the non-motion duration cap. Packet stages, configured effects,
the 20-percent random hit visual, sample 119, and event four follow in the
same order as retail. The result deliberately leaves live state mutation,
effect ownership, audio playback, and equipment synchronization to later
world owners.

The player's owned companion does not reuse either receiver. Its virtual
callback is `0x0045f9f0`, selected from the type-five companion vtable at
`0x00476e38`. It rejects companions with no life and presentation actions
7, 8, and 10 before reading the packet or consuming randomness. The defense
profile is family one: character number, physical defense, magical defense,
and the companion's native element. A non-positive packet base again skips
the shared formula and becomes zero damage.

Only the client whose local player slot equals `companion character number %
10` changes companion life. Lethal damage selects presentation action six,
resets its counter, and locks the action. Unlike the enemy receiver there is
no damage attribution array, source status 73, reflection, kill-status award,
or network request in this callback.

A surviving companion uses tables 24 and 25 like the enemy family, but has no
separate reaction-defense or always-suppress-displacement fields.
Player-family packets use
the native element's opposing packet strength to select table 24; other
packets use row five when packet element is opposing. Effect-family chance,
duration, and displacement suppression come directly from the native-element
banks. A reaction that permits displacement is capped to 15 updates before
packet word 76 is added,
then presentation action five stores the impact angle and optional facing.

Packet reaction stage one creates effect 21015 through 21018 with owner kind
four and plays sample 119; stage two only changes the reaction stage. The two
configured packet effects and the 20-percent random hit effect instead use
owner kind two. That otherwise easy-to-miss constructor difference is
preserved in the portable request. Death action six and default event four are
selected after those common effects. As with the other receivers, live actor
mutation, effect allocation, and audio playback remain consumers for the
later complete combat loop.

The second table row is the value consumed by `0x00450d40`. It is 128 for both
new characters, producing movement tier five. This is now read through the
portable `RKC_RPG_TABLE` boundary and owned by `PlayerData`; `PlayerActor`
receives only the resulting movement tier and no longer owns level or other
character data.

Retail saves begin with `ShadowFlare0005` and the same plain 0x160-byte record.
The load menu reads this copy directly. The complete record is repeated inside
the obfuscated payload and restored by `0x0044cac0` along with dynamic objects
and world state.

`0x0044b580` writes the payload size first, followed by one byte from the
Visual C++ `rand()` sequence and a four-byte signed-byte checksum. It XORs
each plain payload byte with that random byte, then replaces the result with
its index in the same 256-byte permutation used by the retail data path.
OpenShadowFlare now writes that envelope and performs the inverse operation
when preserving an existing save. It also follows the retail variable-sized
item stream to restore and rewrite the nine known equipment slots, backpack,
the belt, and the 9-by-10 Special Item owner. Grid positions, Gold quantities,
durability, identified state, and all still-unnamed instance bytes survive
the round trip. The extra equipment records and unknown trailing payload bytes remain
untouched. Loading also walks the next counted state array and restores the 51
transport flags against Table 40. The rest of the dynamic payload is still
pending.

Primary-button input has two retail behaviors. A press and release is a
latched destination click. Keeping the button down continuously replaces the
destination with the live pointer position, but releasing after that held
state cancels movement immediately.

`0x004562f0` makes the distinction explicit in each 28-byte input record:
offsets `+0x04` and `+0x08` are the previous and current down states, `+0x10`
is the press edge, `+0x14` is the release edge, and `+0x18` counts updates
while the button remains down. `0x00441c00` does not arm release cancellation
until that counter is greater than nine. A normal click can therefore span
several game updates and still latch its destination. The pointer target is
refreshed on every down update either way.

The portable gameplay state now preserves that ten-update threshold rather
than classifying any click longer than one 30 Hz update as a hold. A press
consumed by a speech bubble stays UI-owned until button release, even when
selecting the option closes that bubble. It cannot be reinterpreted as a held
ground command on the following update.

Portable gameplay still updates at the retail 30 Hz cadence, while the window
is presented at 60 Hz. Rendering the current simulation snapshot twice made
camera scrolling visibly step at a constant rate. The runtime now keeps the
previous and current actor positions and interpolates only their render
positions and the camera between updates. Collision, scripts, animation
counters, and all other game state remain on the fixed 30 Hz clock.

The variable section at `0x324` begins with counted object, PEOPLE, and enemy
resource preload lists. Four counted runtime groups follow in a fixed order:
object, PEOPLE, enemy, and item. Their common record contains IDs, optional
names and colors, label height, position, judgement, direction, initial CAF
part overrides, optional fixed-capacity part/color arrays, and one preserved
unknown value.

The object tail is `0x34` bytes. `FUN_00429600` creates its type-zero
`0x120`-byte runtime class through `0x0045dca0`; its character number is local
ID plus 10,000,000. The initializer at `0x0045dd00` copies the transformed
tail. Tail value zero chooses between a static `Pattern.Njp` index and a CAF
chart stored by values one and two. The remaining confirmed values feed draw
status bit `0x80`, object height, draw flags and strength, and red/green/blue
strengths into `0x0045ddd0`. Unknown values remain lossless in
`ScenarioObject`.

Remote Town contains seven of these dynamic objects, with local IDs `0`,
`200` through `204`, and `300`. The final one is the named Warehouse object.
They use object resources 8, 15, and 14 from the first preload list. These
records are separate from the map's static OBL scenery. Across all 209 retail
MCT files, the exact sequential decoder reaches 5,203 object and 163 PEOPLE
records without a resource-list mismatch.

Object 200 and the Warehouse at local ID 300 are the first reconstructed
type-zero pointer actions. The normal world pointer tests opaque cells in
their static NJP or current CAF display, then uses the common judgement-box
distance before running status kind zero for script characters `10000200` and
`10000300`.

Object 200 reaches opcode 37 at `0x004334da`. Its argument zero sets the
transport UI owner used by `0x0040c950`. That renderer draws Status pattern 13,
compacts enabled Table 40 rows into ten entries per page, uses patterns 22
through 24 for rows and patterns 11/12 for paging, and plays sample 58 on
navigation or selection. Table 40 row zero is `Remote Town`, scenario zero,
entry 50. The same-scenario branch at `0x00426200` combines that entry value
with local player zero as `entry * 4`, selecting MCT entry key 200 at
`(94685,-2756)`, direction 7.

The complete `0x00426200` call takes player number, scenario ID, entry value,
an auxiliary transition flag, an optional explicit position, and an entry-key
player override. With a nonnegative entry value, both the same-scenario fast
path and the full load path query:

```text
entry key = local player number + entry value * 4
```

An entry value of `-1` uses the supplied world coordinates instead. A changed
scenario saves/releases the old scenario state, changes music when needed,
rebuilds map and dynamic resources, loads that scenario's `Scenario.Njp`, and
then applies the entry. The portable fresh-world loader now accepts the first
three values explicitly and resolves scenario directories with the retail
decimal `%08d` spelling. Scenario 6, entry value 4 is covered against MCT key
16.

Portable ownership now follows the teardown boundary visible in this routine.
`ScenarioWorld` owns the current MCT and SCS data, GND and OBL state, map
patterns and overview, exploration mask, scenario objects, PEOPLE actors, and
ground items. Player data, equipped and carried items, belt contents, Special
Items, quests, missions, transport flags, and common resources stay in
`WorldScene`. Types 10 through 13 script values live with that persistent
owner as the retail global and local-player arrays do. A scenario is prepared
as a temporary complete owner and only then handed to the interpreter runtime,
whose callbacks remain at a stable address.

The live path now uses the same transaction. Same-scenario travel resolves the
entry and relocates without rebuilding resources. A changed scenario commits
only after all data and visuals have loaded; failure keeps the old map, script,
player, items, progress, and music usable. A successful commit clears stale
pointer, interaction, ground-item, and pending-audio state, preserves the
player-owned and progress owners, adopts the new SCS, relocates to its entry,
switches BGM, and starts the later standard loading presentation. Explicit
coordinate entry `-1`, the alternate `VisualNN` presentation, multiplayer
ownership, and exact teardown ordering remain.

The first authored cross-map path is now traced end to end. During the
scenario update at `0x004305d0`, status kind three resolves its character to a
live MCT entity and calls the inclusive rectangle test at `0x00414350`
against the local player. This does not consult the entity's three ordinary
state channels. Remote Town object zero is consequently an invisible trigger
at `(90124,4275)` with bounds `[-106,66,964,604]`, even though its visible and
judgement channels are zero.

Its sentence 219 calls opcode 17 at `0x00432162` with scenario 1 and entry
zero. The handler stores both values in the pending transition record, enables
the request, and resets the explicit-position selector to `-1`. The portable
world defers the actual transaction until the interpreter has returned, then
publishes one scenario-change event to the runtime. This keeps the SCS owner
valid during command execution. Once that synchronous transaction has
finished, the runtime resets map-local UI and camera state, changes music, and
presents the new world immediately.

Scenario 1 is `Near the Remote Town`, map `f00_02`, music track 1. Entry key
zero places the local player at `(90581,5288)`, direction 7, with the camera
still anchored at `(320,240)`. The MCT creates 48 objects and 127 enemies.
Its object-zero kind-three trigger calls the same opcode with `{0,0}`, landing
back at Remote Town's `(89898,2811)`, direction 3, and music track 0.

The Warehouse reaches opcode 41 at `0x004335ac`. Argument zero toggles runtime
flag `0x0048ce48`, the same one-page Special Item owner handled by
`0x00447970`; it is not a separate warehouse container. Nonzero opcode-41
arguments select a related ten-page owner which is not exercised by Remote
Town and remains pending.

The portable decoder also reads all seven Remote Town PEOPLE records and their
bounded-wander tails. The first value after the bounds is copied to runtime
offset `+0xd4` and gates native action 21's target-facing branch. The next is
inverted by the loader and controls autonomous wandering at `+0xd0`. The
final value, `-65` for all seven records, remains unnamed. Enemy and item
records are now structurally traversed at their retail `0x13c` and `0x10` tail
sizes, so the entry table and three footer values are read in forward file
order. Their tail meanings and runtime actors are still open.

All seven people records are instantiated from that table. Resource lookup at
`0x00455ee0` resolves each ID to its zero-padded `Character\PEOPLE` directory;
the four animals share resources `01000000` and `01000001` exactly as named by
the MCT. The first record creates Ostare through the type-one path constructed
at `0x0045d020`. `0x0045d620` draws idle chart zero using MCT direction 7 and
advances its frame counter once per game update. After the tail's 30-update
pause, `0x0045d150` starts movement-controller mode three. That mode chooses an
inclusive random point inside the spawn-relative rectangle through the shared
selector at `0x00454310`, while
`0x0045d9f0` draws chart one and moves at 10 world units per update until
arrival or the tail's 30-update limit. The MCT's custom mask disables parts 4
and 5, leaving the shadow and two visible frame-zero cells rather than drawing
every CAF layer.

`0x00454310` handles seven destination modes before the collision and stepping
code at `0x00454930`:

- mode 0 keeps a fixed point;
- modes 1 and 4 approach a scenario actor or player;
- modes 2 and 5 retreat from a scenario actor or player;
- mode 3 chooses independent inclusive X and Y coordinates in a rectangle;
- mode 6 projects toward the edge of a rectangle after a 30-degree rotation.

The target modes compare judgement bounds rather than actor origins. A zero
refresh interval becomes one; otherwise the old destination is retained until
the authored interval. Every refresh consumes the percentage draw, including
a zero-percent turn chance. A successful turn consumes a second draw and
rotates by `rand() % 2001 - 1000` steps of 0.0010471973333333333 radians.
Retreat selects a point one unit beyond the stop distance. Mode 2 then returns
no movement despite updating that stored point, a native quirk preserved by
the portable selector. Mode 6 uses signed floor midpoint rounding and native
truncate-to-zero projections. Destination selection remains separate from
collision, path stepping, and animation.

All seven Remote Town records use this type-one PEOPLE class, but the two tail
flags produce different behavior. Only Ostare enables autonomous wandering.
Ostare, Syria, and the four animals allow native action 21 to face its
evaluated target; Malse deliberately ignores that turn request. Native action
18 separately stops the current walk and enters interaction state, while
action 19 releases it. Keeping actions 18 and 21 separate is important:
scripts may suspend an actor without changing its direction.

Player CAF parts are not independent actors that should all be drawn.
`0x00444ca0` rebuilds an enable table on every appearance refresh: entries 0
and 1 are the base body and shadow, while equipped items select additional
armor and weapon entries. The new-player MCT record starts at direction 3, and
`Animation00.Sdw` supplies the corresponding one-bit player shadow.

The same MCT stores music index 0 for Remote Town. `0x004275e0` maps scenario
music indices to `System\Game\Music\BGM%02d.Voc`, loads the selected container
into voice slot 500, and resets its start counter. `0x004275a0` starts sample
zero looping on the following gameplay update with the configured BGM volume.

## Gameplay HUD and pointer

The fixed gameplay interface is drawn by `0x004039f0`, after
`0x004030f0` has submitted the camera-driven world. Gameplay resource loading
at `0x004127d0` assigns `System\Game\Pattern\Bar.njp` to UPD slot 5.
Patterns 7 and 8 form the fixed y=400 through y=479 bar, pattern 10 marks run
mode, pattern 11 marks walk mode, and pattern 15 frames the experience bar.
Patterns 19 through 28 are the level digits.

Life uses patterns 0 through 2 at x=81, y=426. Mana uses patterns 3 through 5
at x=106, y=453. Both live fills are 206 pixels wide and use
`current * 206 / maximum`, preserving one pixel for a positive value which
would otherwise truncate to zero. Packet clip rectangles reveal the live
portion without scaling the artwork. The other patterns provide delayed
damage/healing colors, particles, conditions, companion state, and later
values which still need their gameplay owners.

The executable registers `IDC_ARROW` once in the window class and contains no
later `SetCursor` call. Hover, selection, and click state are therefore drawn
as world feedback; the pointer itself remains the normal platform arrow.

`0x00416bb0` draws the ordinary gameplay feedback as four one-pixel lines.
The configured range selects half-sizes `0`, `12`, `16`, `24`, or `48`; the
shipped default is the 16-pixel choice. Empty ground uses white at strength
100, any target raises it to 300, and a ground item changes it to
`(224, 224, 0)`. The square is not submitted below y=407 and disappears while
a modal gameplay message owns input.

## In-game settings

`0x004103c0` draws the Escape menu with patterns 59 and 58 from `Status.njp`;
the first is the half-transparent fill and the second is the opaque frame.
Its six original boolean rows begin at y=86 and advance by 16 pixels. The
portable game deliberately leaves the screen-mode row at y=86 blank and always
uses an LWL window, so the first visible setting remains at the retail y=102.
ON occupies x=376 through 425 and OFF occupies x=426 through 463.

The five click-range values occupy 24-pixel cells beginning at x=317 and y=182.
The priority row below them displays classes from priority four down to zero.
Clicking a class shifts every lower class up by one and moves the selected
class to priority zero, which is why it moves to the right-hand end rather
than simply swapping with its neighbor.

The effect and BGM tracks use Status patterns 120 and 68 at y=223 and y=243.
The first 200 slider positions cover DirectSound values `-3000` through zero;
the far-left position is the separate mute value `-10000`. BGM changes update
the playing world voice immediately. Escape closes the panel and ordinary
world input remains suspended for every frame owned by it.

The Help row at y=286 switches to `0x0040e710`. Status pattern 10 is the
640-by-415 authored frame; pattern 66 supplies the 230-by-128 ground preview at
`(64, 70)`, surrounded by the original half-opacity one-pixel outline. The
player is drawn at `(152, 148)` with CAF chart 7. The mouse and keyboard tables
are not rewritten descriptions: their original text, colors, 6-by-12 font
cells, two-pixel keyboard line spacing, and coordinates are preserved.

Opening Help from the settings row also starts the common modal-tab animator
at `0x004088b0`. Status patterns 27 through 30 slide the `CLOSE` tab from
y=413 to y=393 and then pulse in the retail eight-phase order. Escape or a
click above y=412 closes Help. The `H` shortcut opens the same reference page
directly. The companion preview is conditional in retail; the portable
renderer will add it when the player-owned companion system exists.

The Map row at y=270 and the `N` shortcut switch to `0x0040d4d0`. The screen
keeps the right half of the world visible and clips its own content to
`(32, 40)` through `(318, 374)`. Pattern 0 from the current scenario's
`Scenario.Njp` is positioned in one-tenth real-screen coordinates, with the
local player held at `(160, 210)`. A separate map-sized mask begins black.
Gameplay clears a 68-by-46 rectangle in that mask around every visited
position, which reveals the overview without exposing unexplored territory.

`MapIcon.njp` patterns 0 and 1 form the local `PLAYER.` marker. It is visible
for 15 updates and hidden for five. Arrow keys change the map origin by 16
pixels horizontally or 10 vertically; Enter clears both offsets. Status
pattern 71 supplies the authored half-screen frame and pattern 118 pulses over
60 updates. The scenario title is drawn at `(72, 50)` over its half-opacity
label backing.

Unlike Help and the Mission List, the Map is not a pausing modal. Its active
UI flags are `0x11`: the left 320 pixels belong to the Map while the world
keeps updating in the right 320-pixel viewport. The camera anchor moves from
`(320, 240)` to `(480, 240)`, so the local player stays centered in the visible
half, and mouse picking uses the shifted projection. Clicks in the Map panel
do not become world commands. Escape or `N` closes it and restores the normal
camera anchor; the secondary-click handler at `0x0044ad80` also closes it for
clicks above the HUD. Retail loads and saves the exploration masks as
`Save/M%08d%02d.msk`; portable
per-save mask persistence and the three other online-player markers remain
to be connected.

The Save and Return row occupies y=302 through 313, and Save and Exit occupies
y=318 through 329. They enter confirmation states two and three. Both dialogs
draw their prompt at y=170, with YES at `(336, 202)` and NO at `(384, 202)`.
Opening a dialog and accepting it use sample 56; NO uses sample 55 and returns
to the ordinary settings state. After YES, states one and four draw
`Now saving the data ` for one update. A successful state-one save then
transitions to the title, while state four ends the game.

When `Save Image at Game End` is enabled, the render loop first draws world
layers 20, 19, and 18 into a separate preview surface. `0x0044e830` writes the
centered player view beside the save as `Save\%04d.Bmp`. The bitmap is 391 by
114 pixels, uncompressed 24-bit BGR, and does not include the HUD or Escape
menu. The load screen places it at `(224, 60)` without scaling.

## SFWindow Object Layout (at 0x00482778)

```
+0x000: HWND hwnd
+0x004: HMENU hMenu
+0x008: char className[256]        "SHADOW_FLARE"
+0x108: char windowTitle[256]      "ShadowFlare for Window98/Me/2000"
+0x508: WNDCLASS wndClass (40 bytes)
+0x534: int windowFlags
+0x584: RKC_DIB cursorBitmap
+0x59C: int gameState              (0, 1, or 2)
+0x5A0: RKC_UPDIB* pUpdib
+0x5A4: RKC_RPGSCRN* pRpgScrn
+0x5A8: RKC_DBFCONTROL* pDbfCtl
+0x5AC: void* pUnknownObj
+0x5B4: RKC_NETWORK* pNetwork
+0x5B8: RKC_DSOUND* pDsound
+0x5BC: State0Handler object
+0x620: State1Handler object
+0x684: State2Handler object
+0xB144: void* pUnknownObj2
+0xB154: HIMC immContext
```

## Shutdown Sequence (0x00401b90)

1. RKC_DBFCONTROL::StopAll()
2. Wait for drawing complete (spin on drawing flag)
3. State handler cleanup (at +0x5BC, +0x620, +0x684)
4. RKC_DSOUND::Release()
5. ImmReleaseContext()
6. Destroy DSOUND, NETWORK, DBFCONTROL, RPGSCRN, UPDIB
7. Destroy object at +0xB144

## Key Function Tables

### Initialization
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x004022e0 | WinMain       | Entry point, message loop |
| 0x00401550 | CheckSingleInstance | Mutex-based single instance check |
| 0x00401eb0 | LoadConfig    | Load SFlare.Cfg |
| 0x004014a0 | ProcessCommandLine | Parse command line args |
| 0x00402520 | CreateGameWindow | Register class, create window |
| 0x004016b0 | InitGame      | Initialize all subsystems |

### Game Loop
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x004023d0 | UpdateGameState | State machine dispatcher |
| 0x00420c40 | State0_Init   | Initialize title/menu state |
| 0x00420df0 | State0_Update | Update title/menu |
| 0x00421a00 | State1_Init   | Initialize loading state |
| 0x00421bf0 | State1_Update | Update loading (cleanup) |
| 0x0041d3f0 | State2_Init   | Initialize gameplay state |
| 0x0041d880 | State2_Update | Dispatch gameplay logic |
| 0x0041d970 | State2_Main   | Main gameplay (2936 bytes!) |

### Input Handling
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00402b30 | WndProc       | Window message handler |
| 0x00402840 | KeyDownHandler | VK_RETURN, VK_SNAPSHOT |
| 0x004026d0 | KeyUpHandler  | (stub) |
| 0x004028a0 | LeftClickHandler | Mouse left button |
| 0x00402900 | RightClickHandler | Mouse right/middle |

### Scenario/Loading
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00426200 | LoadScenario  | Main scenario loader (4846 bytes) |
| 0x00426160 | LoadScenarioData | Helper for scenario loading |
| 0x004021b0 | FindSaveSlot  | Find next free save slot |

### Major Functions (by size)
| Address    | Size (bytes) | Description |
|------------|--------------|-------------|
| 0x00429ec0 | 20119 | CommandDispatcher - Command/Event dispatcher (huge switch) |
| 0x00430f80 | 13677 | ScriptInterpreter - opcode values 0x00 through 0x4b |
| 0x00462f80 | 9247  | LoadItemData - magic SFItemDataV0000 |
| 0x004103c0 | 7773  | OptionsMenu - Settings menu |
| 0x0044cac0 | 7527  | LoadGame - load save file, XOR decrypt |
| 0x00414990 | 7221  | ObjectNpcDisplay - via RKC_RPGSCRN_OBJECTDISP |
| 0x00427b50 | 6780  | LoadScenarioMct - magic MCED DATA v0000 |
| 0x0040aed0 | 6758  | ItemStatsDisplay - format item stat bonuses |
| 0x0044b580 | 5429  | SaveGame - write save file, XOR encrypt |
| 0x00409a60 | 5212  | StatusScreenDisplay |
| 0x00405750 | 4936  | CharacterStatusDisplay - element bonuses |
| 0x00426200 | 4846  | LoadScenario - main scenario loader |
| 0x00423ca0 | 4306  | ScenarioLoader_Phase2 - called from State1 |
| 0x00446320 | 3898  | (UI/input related - needs analysis) |
| 0x0041afc0 | 3680  | NetworkServerHandler - RKC_NETWORK packets |
| 0x00441c00 | 3462  | (needs analysis) |
| 0x004039f0 | 3437  | Gameplay HUD, bars, level, and status packets |
| 0x0041d970 | 2936  | State2Handler_Main - main gameplay update |
| 0x00421e10 | 2710  | CharacterCreation - class/gender select |
| 0x004239b0 | ~500  | LoadSaveSlotInfo - read save headers |
| 0x004021b0 | 129   | FindSaveSlot - find free save slot |

### Cleanup
| Address    | Function Name | Description |
|------------|---------------|-------------|
| 0x00401b90 | Shutdown      | Release all subsystems |

## Player attack impact

The CAF marker now enters the direct player-impact path from `0x00439140` and
`0x00435e60`. `0x00413e00` compares derived player hit rate at `+0x1bc` with
the enemy initializer's pre-AI word 10, clamps the difference to `20..98`,
then consumes exactly one `rand() % 100` roll. A miss stops there. A hit calls
`0x00417e70`, then fills the family-zero packet with the local player number,
derived physical attack and defense, eight affinities, the 17 persistent
words copied from runtime `+0x1dc`, level, reaction modifiers from main-hand
instance parameters 16, 14, and 15, and the weapon identifier.

The first hit-effect draw selects `21000..21003`. `0x0044f990` then always
consumes the reflection roll, summing active equipment instance parameter 20
as its chance and parameter 21 as its returned percentage. Main-hand subtypes
8 and 9 consume one later draw and replace the effect with `21004..21006`.
The target receiver runs next. Sample 6 is queued after it returns, and only
then does an occupied main hand consume its 30-percent durability roll.
Weapons with zero maximum durability still consume that roll but do not lose
condition.

The live owner commits the receiver's life, per-player attribution, reaction,
event, and defeat fields back to `EnemyActor`. It deliberately reuses
`resolveEnemyDamage`, including its tables and conditional random draws.
Broken category-zero and category-one equipment remains present and keeps its
weight, while `0x0044ea60` proves that its base derived contributions and
element strengths no longer participate. Hit/death CAF presentation,
reaction displacement, and ordinary configured effect-list ownership now run
at the live actor boundary described above. Specialized effect dispatch
families remain separate follow-up work.

## Enemy effect controllers and runtime actors

Enemy effect presentations enqueue a controller request; they do not create a
projectile directly. `0x0042fdc0` stores the request in a `0x3b0`-byte node,
including both actor identities, the direction, optional origin and source
judgement, the complete combat packet, the authored delay, and the remaining
constructor values. The list updater at `0x0042fd60` keeps that node until the
handler selected by `0x00429ec0` returns zero.

All twelve nonnegative effect types found in the shipped enemy records have a
specialized branch. The dispatch covers types 1, 2, 3, 4, 5, 10, 11, 12, 13,
14, 16, and 21. This is separate from the much simpler 21000-series receiver
visuals already owned by `CombatEffectActor`.

Types 1 and 2 establish the controller/actor boundary. Their first controller
update creates source resources `10000012` and `11000027` respectively at the
source actor's then-current position. At constructor delay 12, the handler
resolves that position again and creates a second actor 180 world units along
the stored angle. The type-1 child uses resource `10000010` and positional
sample 19; type 2 uses resource `10000040` and sample 94. Both use
`[-50,-50,50,50]` judgement, retain the packet, use CAF chart zero, and expire
when their static environment sweep collides. The controller expires after
creating the child, independently of both visual actors.

Type 3 is the first staged area controller. `0x0042b540` reads row zero and
column `subtype - 1` from Table 205; shipped Plasma Bat subtype 20 resolves to
five waves. Beginning at constructor delay 12, one wave is attempted every
four updates. Its position is projected from the stored impact origin, not
the enemy's later position, at radius `wave * 200 + 250` along the stored
angle. A `[-100,-100,100,100]` judgement check includes map collision and
live type-zero scenario objects. The first failed placement sets a persistent
stop flag, suppressing that wave and every later wave without consuming a
random value.

Each clear wave consumes one `rand() % 4` chart choice and creates resources
`10000030`, `10000031`, and `10000032` at the same position. The first actor
uses that random chart, applies the copied packet to every overlapping target
only on its first update, and does not expire after the first target. The
other two use chart zero and are visual layers without an active collision
window. All three use direction eight, their selected chart's full lifetime,
and the same 100-unit judgement. Positional sample 21 is emitted once per
clear wave. The controller remains alive until `delay + Table205Value * 4`,
independently of the actors it has already created.

`0x00429dd0` creates those children in the category-50000000 actor family.
`0x0045e1a0` installs a 126-word descriptor, and `0x0045e1e0` owns their
common update. That update supports homing, free, and owner-attached movement;
static environment sweeps; an inclusive target-collision window; independent
player/companion/enemy target bits; exact target IDs; living and
current-scenario checks; optional previous-hit storage; physical or magical
evasion checks; receiver callbacks; and configured positional audio. It then
draws either the descriptor's static pattern or CAF chart and applies the
authored lifetime. The nearby action dispatcher at `0x0045f960` belongs to the
separate category-40000000 actor class; it is not part of this generic effect
actor.

For types 1 and 2, descriptor offset `+0x1c` gives the source actor one complete
chart-zero/direction-eight lifetime and leaves the traveling child unlimited.
`+0x40` enables the static environment query, `+0x44` makes only the child
expire on contact, `+0x48` carries its distance per update, `+0x54` carries
display height, and `+0x58` selects chart zero. Target collision starts at
update zero for the child through `+0x8c`; the source uses `-1` and never opens
that window. `+0x94` enables drawing. These fields explain the two actors
without inventing a visual “action one.”

The portable implementation must preserve both lists. A controller is
simulation state that can create several actors over time; a runtime actor is
renderable state that can move, collide, and dispatch an impact. Treating an
enemy request as one short CAF would make the picture plausible while moving
the actual effect, sound, and damage to the wrong update.

`EnemyEffectController` now ports the complete controller half of types 1, 2,
3, and 4 without pretending that every specialized family is finished. It emits the
source actor on update zero, re-resolves the source at the exact authored
delay, projects the second actor with the retail Y-axis convention, copies the
combat packet, and places sample 19 or 94 at that second position. A zero delay
creates both actors in one update, a negative delay remains active, missing
owners resolve from zero, and omitted origin or judgement pointers do not leak
stale values.

The same owner now keeps type 3's table-driven wave counter, fixed origin,
persistent placement stop, random damaging chart, two visual companions, and
sample 21. Passive timing and obstruction tests are paired with the shipped
Plasma Bat in scenario `00010001`; the live case renders all three resources,
passes the first layer through the ordinary player receiver, and preserves
the player's item owners.

Type 4 (`0x0042a860`) follows the source rather than storing a fixed origin.
On update three it resolves the source position and creates resource
`10000002` with the source judgement, chart zero, direction eight, and
additional display status `0x80`. On the authored delay, which is ten for the
shipped request, it resolves the source again. Resource `10000000` is then
created at each opposite judgement corner with display height 200: the first
actor draws chart one, while the second draws chart zero but deliberately
uses chart one's frame count as its lifetime.

That delayed update also plays positional samples 29 and 23 and creates an
invisible one-update actor. Its judgement expands every source edge by 150,
its collision window is update zero only, and it passes the copied packet to
every eligible target while using the common bank-zero contact sample 20.
The controller expires immediately after those three actors have been handed
to the runtime list. When the local player is less than 3001 world units from
the resolved source, the same update requests mode zero camera shake:
duration eight and magnitude six. `0x00412690` stores the request and
`0x00412720` alternates a zero/six vertical world offset until the eighth
update, after which the camera is steady again.

`RuntimeEffectActor` now ports the next shared parts: chart-zero source
lifetime, free movement from the immutable spawn point, the zero-distance
first update, retail integer projection, static OBL/GND sweeping, the
special-environment exclusion bit, contact expiry, inclusive target-window
timing, CAF frame scaling, and interpolated render snapshots.

The target query is deliberately separate from the movement sweep. At the
start of an active collision-window update it intersects the actor's current
judgement rectangle with dynamic objects in display-query order. Mask bits
`1`, `2`, `4`, `8`, and `16` select players, locally owned companions,
enemies, NPCs, and scenario objects. Exact identity, life, owner, active,
display, runtime-state, and self filters run before either every eligible
target is processed or the first nearest position wins a strict distance tie.

Players, companions, and enemies use packet word 1 to select physical or
magical evasion and packet word 36 as hit rating. The `20..98` chance consumes
one retail random value per processed target even without a receiver packet.
The optional repeat list records an identity before that roll, including a
miss, and stops growing at 500 entries. Typed packet/miss requests preserve
the receiver boundary. Target and object sound pairs share the original
once-per-update guard, including the distinct NPC multi-target spatial mode,
while static contact uses the actor's pre-movement position.

The live world owner now constructs target snapshots, applies typed receiver
requests, queues positional sounds, and renders the runtime actor resources.
Its type-4 support also keeps invisible packet actors outside the renderer,
honors a lifetime chart separate from the displayed chart, carries additional
display status into the retail sort classes, and applies the short camera
shake without changing the portable presentation backend.

## Enemy kill rewards

The lethal callback at `0x004134a0` uses each enemy's MCT pre-AI value 13 as
its experience reward. Per-player attributed damage selects Table 14 row
`damage * 10 / maximum life`, clamped to 0 through 10, and that percentage is
applied to the reward. A direct local-player kill increments the persistent
total at player-record offset `0xb0`; ordinary main-hand subtypes select the
eight counters beginning at `0xb4`, while effects and other sources use
`0xd4`. The experience total lives at `0xd8`, so the existing 0x160-byte
player record already carries all of these values through save/load.

Table 13 supplies the threshold for `level - 1`. Reaching it discards overflow,
records the current job in the 100-byte history at `0xdc`, applies the matching
gender-specific 900-series growth column to the 13 base parameters, refreshes
life and mana, and resets experience. New-character initialization fills that
history with novice job `0x10`, matching `0x00440f70`. The full later
job-selection and skill-unlock branch still needs its own class-system slice.

Enemy death action 11 calls `0x0045a000` before `0x0045a030`. The first uses
MCT pre-AI value 14 as a Table 30 row. Every attempt consumes the chance draw;
a success consumes a separate ten-slot profile draw, applies Table 31's
category, level, variant, and episode filters, then uses `0x00401520(9)` for
the weighted item offset. Constructing the definition rolls 39 instance and
eight element triples. Successful objects are placed around the enemy at
radius 200.

The Gold callback reads MCT post-AI values 26 through 28. Equipped instance
parameter 26 changes the 100-percent multiplier, the chance comparison is
strict, and the inclusive amount draw occurs only after Gold construction.
Its origin is the enemy position plus the judgement left/top corner and 100
world units on y. Both paths create complete owned item instances through the
ordinary ground-item owner. The next PRNG draw then chooses death effect
21010's direction, and the first bounce produces the existing category
landing sound.

## Level-up notice

`0x00450fb0` creates the level-growth text as an auto-sized text owner with
four pixels of padding. Flag `0x80` centers its rectangle in the 640 by 416
play area; its background is black at opacity 250 and its text color is
`224,224,224`. `0x00451a40` draws a separate one-pixel white frame at opacity
500 and counts down the 900-update lifetime.

The notice stays centered through counter 840. During the next ten updates its
stored initial position is interpolated to x `640 - width`, y 1, after which
it remains in the upper-right until expiry. `0x00451cb0` ignores notice clicks
for the first 30 updates. A later click inside the notice releases it and
consumes the input before ordinary world interaction.
