# ShadowFlare's script engine

ShadowFlare does not hardcode every conversation, quest, and town event in the
main executable. Scenario behavior is split between data and engine code:

```text
Scenario.Mct       map, music, actors, entry points, and basic actor settings
Scenario.Scs       flags, messages, status triggers, sentences, and commands
RKC_RPG_SCRIPT     binary script container and lookup boundary
ShadowFlare.exe    interpreter, operand domains, command behavior, and native actions
```

That boundary matters to the reconstruction. A dialogue line, quest branch, or
actor instruction that already exists in an SCS file should stay in the SCS
file. The portable executable should decode and interpret it, not copy it into
`WorldScene` as new C++ logic.

This document records what we currently know. It will grow along with the
interpreter.

## The other behavior systems

`Scenario.Scs` is the game's only known general-purpose scenario bytecode, but
it is not the only data that controls behavior. The retail files divide that
work into four layers:

| Layer | Retail data/code | Responsibility |
|---|---|---|
| Scenario script | One `Scenario.Scs` in each of the 209 scenario directories | Dialogue, quest branches, flags, messages, rewards, services, spawning, and scenario events |
| AI action data | `System\Game\Parameter\Control.aid` | Reusable actor and enemy behavior choices, grouped by event and guarded by conditions |
| Scenario setup | The matching `Scenario.Mct` | Map, music, entities, positions, movement areas, entry points, appearance, and initial actor settings |
| Native game logic | `ShadowFlare.exe` and the gameplay DLLs | Evaluating conditions, executing script opcodes and AI actions, movement, combat, rendering, audio, and other engine services |

The distinction is important. MCT values and AID records are data-driven, but
they are not SCS sentences and do not run through the scenario interpreter.
Conversely, an NPC conversation or quest branch found in SCS should not be
recreated as an AI action or hardcoded state machine.

### `Control.aid`

The game ships one global AI database at
`System\Game\Parameter\Control.aid`. Scenario MCT headers name that controller
file, and actors select one of its behavior lists. The reconstructed
`RKC_RPG_AICONTROL` DLL proves the following binary organization:

```text
RKC_AIDATA v001 + 0x1a
64 behavior lists
18 event buckets per list
```

Each behavior list contains:

- a variable-length name;
- a walk-point speed in version 1;
- eighteen event buckets;
- zero or more action candidates in each event bucket.

Each action candidate stores an action number, a 36-byte parameter block, and
a 24-byte condition block. The file therefore describes *which* native action
may be selected and supplies its tuning values. It does not contain SCS-style
messages, sentences, typed operands, nested calls, or arbitrary opcodes.
Recognizable embedded names include behavior families such as `HITandAWAY`,
`GUARD`, and `MAGIC`.

The executable contains the evaluator and the action implementations. The
evaluator at `0x0045c9f0`:

1. selects an event bucket from the actor's behavior list;
2. rejects candidates outside their inclusive life-percentage or target-range
   conditions, with `-1` meaning an open end;
3. clears the retained candidates whenever it finds a new highest priority,
   but still inserts later lower-priority candidates at the front of the
   temporary list;
4. performs a weighted random selection while traversing that temporary list,
   which is the reverse of file order;
5. falls back to event zero when events 1 through 10, 16, or 17 select
   nothing;
6. copies the selected action number and complete action record into the
   actor;
7. dispatches that number to native movement, attack, guard, spell, or other
   action code.

The unusual third step is retail behavior, not a simplified priority rule.
The shipped file contains 33 places where a lower priority follows a higher
one in the same event, so preserving it can affect real selections. Parameter
zero is priority and parameter two is selection weight. Condition zero enables
the inclusive percentage test in conditions one and two; condition three
enables the target-distance query using conditions four and five. Timing,
movement, and the remaining values still need consumer traces, so they remain
raw rather than receiving speculative names.

The portable executable now has a separate `RKC_RPG_AICONTROL` static library
which owns AID decoding and lookup. The shipped catalog resolves all 18,788
MCT enemy references against its 64 exact names and keeps all 1,338 candidates
under their original event buckets. An executable-owned evaluator reproduces
the proven filtering, candidate-list quirk, weighted draw, and event-zero
fallback without putting runtime policy into the data library.

It is deliberately not connected to live enemies yet. Enemy life fields,
target querying, and the native action dispatcher are reconstructed behind
that boundary; selected-action storage and presentation completion still need
to land alongside live movement before evaluation can safely alter an actor.
AI lists, probabilities, and condition values must come from `Control.aid`,
not from NPC-specific C++ branches.

The first two native action handlers are reconstructed behind that live
boundary. Action zero at `0x0045c350` holds the normal idle presentation,
uses parameter one as its duration, reports event 11 while waiting, and
returns to event zero at the inclusive duration boundary. Action one at
`0x0045c3c0` alternates a bounded patrol and pause: parameter one limits the
whole action, parameter three is movement speed, parameters four and five are
movement and idle update counts, and parameter six selects the walk chart.
While it remains active it reports event 12, then returns to event one.

The patrol destination is chosen independently on both axes from the
inclusive spawn-relative MCT rectangle. Effective speed is parameter three
times the enemy's MCT speed scale divided by 1,000. A zero movement duration
still enters and immediately leaves the walk presentation but does not choose
a destination or consume random state. The shipped database contains 61 wait
actions and 92 patrol actions, including six of those zero-duration patrols.
This controller remains dormant until its movement and presentation requests
have complete live consumers.

The destination selector below the dispatcher is a shared native game
service, not another script interpreter. Its seven modes cover fixed points,
bounded patrol, player or scenario-actor approach and retreat, and
rectangle-edge projection. Target modes use judgement-bound distance, retain
their old destination until the authored refresh interval, and preserve both
percentage and angle random draws. It only chooses the next destination;
collision and stepping stay in the movement controller.

Target acquisition is shared too. The ranged query checks the four player
slots first and only searches companion character numbers `16000000` through
`16000003` when no player qualifies. Each group chooses the first actor at the
nearest judgement-bound distance, and `-1` leaves a distance end open. The
default query has its own retail activity rules but keeps the same
player-before-companion priority. Both return one typed target record used by
the evaluator and dispatcher; scripts and individual enemies do not maintain
parallel target lists.

Actions two through seven are the two three-variant animated action families.
Their native handlers map them directly to presentation actions one through
six, clear the old presentation, and reset the action counter on entry. The
presentation routine owns targeting and effects, then returns the matching
event number when its animation finishes. Action eight is different: it
resets its action counter but deliberately keeps the presentation already in
progress. The shipped `Control.aid` does not select actions four or eight, but
their executable paths are still preserved. None of these actions drive a
live enemy until their presentation-side behavior is reconstructed.

Actions nine and ten move relative to a target. Nine retreats to a bounds
distance of 10,000 and uses event 14 while active; ten approaches to contact
and uses event 15. Both return their own action number as the completion
event, whether the authored duration expires, no eligible target exists, or
walking stops. Parameter three is scaled movement speed, parameter seven is
target-refresh cadence, and parameter eight is random-turn chance. Player
targets and scenario-actor targets use separate native movement modes.

Action eleven walks toward the cached walk point at the AI list's
`WalkPointSpeed`, stops within 150, and holds event minus one for counters zero
through 90 before returning to event zero. It also completes when walking
stops. No shipped AID record selects action eleven; the executable can force
that path itself. The portable dispatcher now covers all native actions zero
through eleven, but stays off the live actor until its emitted movement and
presentation requests have complete runtime consumers.

### What is not a script

Several other binary files are essential to gameplay but should not be routed
through either interpreter:

- `Scenario.Mct` is declarative scenario and entity setup.
- `Table.Tbd` is a general parameter-table database.
- `Item.Ibn` contains item definitions and resource selections.
- `.Map`, `.Gnd`, and `.Obl` hold map, terrain, placement, and collision data.
- `.Lst` files are map-pattern asset lists.

These formats can drive a large amount of behavior without being executable
scripts. The reconstruction should preserve that data ownership instead of
turning the values into constants.

## Scenario.Scs

An SCS file starts with the 16-byte signature:

```text
ScenaScriptV000\0
```

The rest is a sequence of counted blocks. Strings in the message block are
stored with every byte bitwise inverted and are decoded while loading.

The portable decoder currently reads:

1. temporary flag definitions;
2. network flag definitions;
3. messages;
4. status triggers;
5. sentences;
6. the command and operand records belonging to each sentence.

The decoder checks every count and byte range and requires the complete file to
be consumed. It therefore rejects truncated files, implausible sizes, and
unrecognized trailing data instead of quietly accepting a partial script.
Sentence and message references are checked when they are executed.

Remote Town's `Scenario.Scs` contains:

| Block | Count |
|---|---:|
| Temporary flags | 66 |
| Network flags | 0 |
| Messages | 61 |
| Status triggers | 23 |
| Sentences | 220 |
| Commands | 608 |

## Status triggers

A status record connects a game event to a sentence. Its useful fields are:

- network/local state;
- status kind;
- character number;
- sentence number.

For people loaded from a scenario MCT, the executable derives the script
character number as `12000000 + local people ID`. Ostare is local person zero,
so clicking him looks up character `12000000`, status kind `0`, and enters
sentence `4`. Nothing in the portable world layer needs to know his name,
message number, or sentence number.

Kind `3` is an overlap trigger. The scenario update at `0x004305d0` resolves
the status character to its live MCT entity and passes the entity and local
player rectangles to `0x00414350`. Touching edges count as an overlap. The
sentence runs even when the entity's ordinary visible, pointer, and judgement
channels are disabled, which is how invisible map exits work.

Kind `5` is a periodic scenario update. Remote Town has five such records.
Four keep the town companion actors in sync with the local player's saved
companion type. The player-owned dog is disabled and the other three are
enabled. Each status is an independent callback, so one unsupported periodic
branch must not prevent the later records from updating their actors.

The remaining record is the world-teleporter activation loop. Opcode `34`
measures the judgement-rectangle distance from the local hero to script object
`10000202`. A zero result means the rectangles overlap. That branch writes
`1` to operand type `10`, using the matching Table 40 row as its operand value,
and therefore permanently adds the location to the transport list. The same
shape appears throughout the scenario scripts for all 51 transport rows. Its
nearby fade and visual-packet opcodes are still only partly reconstructed, but
they do not own the unlock.

## Interpreter architecture

The portable implementation lives in
`src/SF_EXE/libs/RKC_RPG_SCRIPT/`. It is a separate static library with two
parts:

- `ScriptData` owns the decoded, immutable SCS data and its lookup helpers;
- `Interpreter` owns execution state, temporary flags, nested sentence frames,
  message waits, and the actor callback attached to an open message.

The interpreter deliberately does not include world, renderer, audio, UI, or
save-game headers. It asks the executable for those services through small
hooks:

- read or write an external operand domain;
- perform a native actor/game command;
- answer a typed query about game-owned state;
- measure the local hero's judgement distance from a script character;
- present a decoded message.

This keeps the old DLL boundary visible without pretending that the original
DLL contained the whole game. It also means the interpreter can be tested with
the retail SCS file without creating a window.

Sentence calls use an explicit frame stack. Opcode 2 presents its message but
does not stop the current sentence: the remaining immediate assignments and
native actions run first, matching the executable. Once that work has
finished, the interpreter reports a message wait. Confirming an actor message
starts status kind `1` for the message's character, which may present another
message and attach the next callback. Unknown opcodes return an error with
their opcode and sentence context; they are never treated as successful
no-ops.

## Commands implemented so far

The retail opcode switch begins at `0x00430f80` and covers opcode values
`0x00` through `0x4b`. Only commands reached by the working Remote Town
interactions are portable so far.

| Opcode | Retail address | Current meaning |
|---:|---:|---|
| 0 | `0x00431005` | Compare two evaluated operands and call a sentence when true |
| 1 | `0x004310a2` | Evaluate an operand and assign it to another operand |
| 2 | `0x00431294` | Present a message, finish immediate sentence work, then wait |
| 5 | `0x0043222b` | Request one of the executable's numbered vendor inventories |
| 6 | `0x004321e8` | Rebuild a numbered vendor inventory from a Table 32 stock profile |
| 10 | `0x00431ca1` | Ask the world to create an item at evaluated coordinates |
| 11 | `0x00431ac5` | Add an evaluated value to a writable operand |
| 12 | `0x00431b0c` | Subtract an evaluated value from a writable operand |
| 16 | `0x00417260` | Ask the world to play an authored sample, optionally range-limited at its evaluated position |
| 17 | `0x00432162` | Queue travel to an evaluated scenario and entry |
| 18 | `0x00431efa` | Stop a PEOPLE actor and enter its interaction state |
| 19 | `0x00431f72` | Native actor action which releases Ostare's interaction |
| 21 | `0x00432094` | Turn a PEOPLE actor toward an evaluated target when its MCT flag allows it |
| 22 | opcode switch | Enable all three state channels for a scenario entity |
| 23 | opcode switch | Disable all three state channels for a scenario entity |
| 24 | `0x00417550` | Ask the world to create authored loot at evaluated coordinates |
| 34 | `0x004337b5` | Measure the judgement-bound distance from the local hero to a script character and write the result |
| 37 | `0x004334da` | Request the transport service selected by the command argument |
| 41 | `0x004335ac` | Toggle an executable-owned item service; argument zero is the Warehouse/Special Item owner |
| 42 | opcode switch | Write the local player's current and maximum life to two operands |
| 43 | opcode switch | Write the local player's current and maximum mana to two operands |
| 44 | `0x00433692` | Write the local player's saved companion type to an operand |
| 48 | `0x00433868` | Select a quest notice and set its counter to 600 |
| 61 | `0x00433f16` | Write the local player's level to an operand |
| 62 | `0x00433f29` | Update a quest's state and trigger its update/completion cue |
| 63 | opcode switch | Write the local player's current and maximum optional condition to two operands |

Opcode 0 stores its comparison selector as a raw operand. The selectors seen
in the executable are:

| Selector | Test |
|---:|---|
| 0 | equal |
| 1 | not equal |
| 2 | greater than |
| 3 | less than |

Opcodes 18 and 21 are separate operations. Opcode 18 addresses a PEOPLE actor,
stops its current walk, and enters interaction state without changing its
facing. Opcode 21 evaluates an actor and a target. Target zero is the local
player; a nonzero value resolves another scenario actor. The actor turns only
when its PEOPLE-tail scripted-turning flag is enabled. This is enabled for
Ostare, Syria, and the four Remote Town animals, but disabled for Malse.

Opcode 17 evaluates a scenario ID and entry value, writes them to the
executable's pending-travel fields, enables the travel request, and clears the
optional explicit-position selector. Remote Town's invisible object zero has
status kind `3`; its sentence 219 supplies `{1, 0}`. Walking through the south
gate therefore loads scenario 1, `Near the Remote Town`, at entry key zero.
That outdoor scenario uses the same mechanism in reverse: object zero supplies
`{0, 0}` and returns to Remote Town's entry key zero.

Opcodes 37 and 41 show why native commands remain hooks. Remote Town object
200 has script character `10000200`; its status-zero sentence calls opcode 37
with argument zero, which asks the executable to open the transport panel.
The named Warehouse is character `10000300` and calls opcode 41 with argument
zero, which toggles the same Special Item owner opened by `X`. The interpreter
does not include panel, input, camera, item, or transport headers. It only
evaluates the command and sends the typed request across its native-command
hook.

Opcodes 5 and 6 follow the same boundary. Scenario status kind 7 initializes
vendor inventory zero with opcode 6 and stock profile zero (or profile 22 for
the alternate global state). Malse later refreshes that same owner with
profile 8, and his `Trade` callback opens it with opcode 5. The script owns
all of those gates and profile numbers; no Malse or Red Goblin condition is
duplicated in C++.

The executable's stock builder at `0x00430c10` expands Table 32 into Table 33
entries. Fixed entries name a category and definition directly. Random entries
filter the decoded `Item.Ibn` definitions by category, episode, level range,
variant flags, and loot weight before creating a normal rolled item instance.
Table 33's start column and row are the beginning of a row-major search in the
9-by-10 merchant grid, not a decorative position. The portable vendor owner
keeps that numbered inventory and placement logic separate from the player
backpack.

Opcode 10 evaluates six operands: category, definition ID, world X, world Y,
minimum quantity, and maximum quantity. Ordinary items create one record at
the requested point. Category `4`, definition `0` is the executable's money
case: it chooses an inclusive quantity, splits values larger than 10,000 into
stacks, and places them around the point at radius 200 with an angle step of
roughly π/10. Ostare supplies the fixed range 200–200, so his opening quest
creates one stack at the first point on that circle.

Opcodes 11 and 12 use operand zero as both the first input and the destination.
They are what turn Ostare's live actor coordinates into the four drop
positions. Opcode 19 addresses the actor again and ends the interaction after
the final bubble closes. Its behavior for other actor types still needs more
examples before the command gets a narrower name.

Opcode 61 is much narrower. The retail handler gets the local player, reads
the level field at offset `0x34`, and passes it to the common operand writer.
The portable interpreter asks its host for
`ValueQuery::local_player_level`, so player data stays game-owned rather than
being copied into the script library.

Opcodes 22 and 23 take a script character number and write one or zero to all
three of its live entity-state keys. Opcode 44 reads player-record offset
`0x140`, which is the currently owned companion type, through a typed host
query. Remote Town combines those commands with the play-mode operand to keep
the selected companion from also appearing as a clickable town NPC.

Opcode 62 evaluates a quest ID, a new state, and a network-notification flag.
Ordinary updates write the new state and issue cue `0x41`. State `2` is the
completion path: the executable latches completion once, requires the old
state to be `1`, writes state `2`, and issues cue `0x42`. Its optional server
broadcast is deliberately left at the world hook because the current scenario
is single-player. The portable `QuestState` owns these executable-level
values; they do not live in the DLL-derived interpreter.

Opcode 48 evaluates one quest ID, stores it as the selected quest notice, and
sets the adjacent counter to `600`. Syria's first new-game conversation
executes opcode 62 with `{0, 1, 0}` and then opcode 48 with `{0}`. The
gameplay interface reads the title from Table 41, wraps it in the retail
Shift-JIS corner brackets, and temporarily draws it just above the lower HUD.
The exact title rectangle is clickable and opens the Mission List. Opcode 62
also plays retail sample 65 for an ordinary quest update and sample 66 for a
valid first completion. While any quest remains in state one, the same retail
function draws `StatusIcon.njp` pattern zero at `(616, 360)`. Its
`616..639` by `368..383` shortcut stays after the timed title has faded and
also opens the Mission List.

Syria's later status-zero branch reads quest zero directly. When it is already
active, opcodes 42, 43, and 63 compare the player's life, mana, and optional
condition pairs before selecting the normal healing or blessing text; it does
not offer quest zero again.

The ordinary Mission List does not store another hand-written copy of this
information. `0x0040cea0` reads the state array written by opcode 62, takes all
48 titles from `Table.Tbd` table 41, and reads mission `n`'s description from
table `700+n`. State zero is absent from the list, state one is unfinished,
and state two is cleared. In the portable code, `QuestState` still owns the
script values while `MissionCatalog` owns the table text; the screen only
combines those two sources for display.

Opcode 2 keeps presentation mode and selection state separate. Operand one is
the writable selection result and operand three is the initially selected
zero-based option. A non-negative initial option and paired `~` runs identify
the actual choice step. The four Remote Town companions use presentation mode
one, while Malse's service menu uses mode zero. The
executable's message layout removes those markers, records the enclosed line
and columns for hit testing, and writes the chosen range number before
entering the actor's status-kind-one callback. Messages with initial option
`-1` are chained informational speech instead: they have no selectable
ranges and close without writing operand one. The portable interpreter and
speech-bubble layout preserve that split.
The native Remote Town fixture walks to Gravity, opens his retail message,
checks the initial red `QUIT` selection, moves the red highlight to
`Check Status`, then hits the rendered `QUIT` range, writes option three, and
verifies that the conversation releases the actor. Unselected ranges use the
retail gray, and leaving all ranges keeps the most recent selection. This
covers the actual world-to-render-to-interpreter path rather than only testing
the marker parser by itself.

Harley's `Explanation` choice exercises the other mode-one path. Choosing
option one shows `1000057` (“You found me finally.”), ordinary acknowledgement
advances to `1000058`, and acknowledging that second bubble reaches native
actor command 19 and releases Harley. Both the interpreter fixture and the
live `WorldScene` fixture cover this complete chain.

The two scripted type-zero services use the same status route. Their MCT
visibility and pointer flags decide whether they can be selected, the common
159-unit judgement distance controls the approach, and only then does status
kind zero run. This keeps Warehouse and transport behavior out of
`WorldScene`: the world owns selection and relocation, while the runtime owns
the panels and their input.

## Operands and variables

Each command operand has a type and a value. The executable's operand reader
starts at `0x004346b0`; its corresponding writer starts at `0x00434920`.

The currently understood domains are:

| Type | Meaning |
|---:|---|
| 0, 1, 2 | Raw immediate value |
| 3 | Runtime-state domain |
| 4 | Scenario temporary flag |
| 5 | Network/state domain |
| 6 | Script character's current world X |
| 7 | Script character's current world Y |
| 8 | Current play mode (`0` single player, `1` client, `2` server) |
| 10 | Persistent transport flags (Table 40 rows) |
| 11 | Persistent script and conversation flags |
| 12 | Persistent quest state |
| 13 | Local-player array |

Type `5` includes three confirmed live scenario-entity ranges. A key beginning
at `100000000` controls visibility, `300000000` controls pointer selection,
and `200000000` controls judgement/collision. The remainder is the entity's
script character number. Type-zero MCT objects use `10000000 + local ID`;
PEOPLE use `12000000 + local ID`.

Several additional types address broader game state. They will be named only
when an exercised retail path gives us enough evidence.

Temporary flags are owned directly by the interpreter and initialized from the
SCS definitions. Persistent and game-owned domains are callbacks because their
lifetime belongs to scenario state, save data, the player, or another portable
DLL boundary. Retail writes types `12`, `10`, and `11` as three counted arrays
immediately after the owned-item stream, in that order. The reconstruction now
restores and rewrites those arrays through their real owners. This includes
Ostare's type-11 flag at index 4, so his opening conversation and starter drop
do not repeat after a save/load. Type `13` is still held for the lifetime of
the player but its later save location has not been mapped yet.

## Working conversations

The first end-to-end slice is Ostare's opening Remote Town interaction:

1. `Scenario.Mct` creates Ostare as local person zero.
2. A click on his rendered actor derives script character `12000000`.
3. Status kind `0` resolves to sentence `4`.
4. The interpreter follows its nested comparisons and sentence calls.
5. Opcode 18 stops Ostare's wandering; opcode 21 separately turns him toward
   the player because his MCT flag allows it.
6. Message `1000000` is decoded from the retail SCS and shown.
7. Return or another click invokes Ostare's status-kind-one sentence.
8. Four more callbacks show messages `1000001` through `1000004`.
9. The third callback reads Ostare's live X/Y position and creates four ground
   items through opcode 10.
10. Closing the final bubble runs opcode 19 and restores world control.

The visible text begins with Ostare introducing himself as commander of the
area. That text is read at runtime from the original data; it is not present in
the OpenShadowFlare source.

Clicking Ostare again follows the next real SCS branch. Sentence six uses
opcode 61 to read the new character's level, selects the under-level-five
path, and shows message `1000005`. Its status-kind-one callback then shows
message `1000006`, resets the temporary dialogue state, and releases the actor.
The interpreter retains the earlier persistent assignment, so this is a
continuation of the same script state rather than a hand-written interaction.

The same table-driven actor path now loads all seven Remote Town people
records rather than stopping after Ostare. Malse's new-game status runs its
real two-message branch (`1000019` and `1000020`) and releases him through
opcode 19. The longer Malse quest dialogue is not forced: the retail SCS only
selects it after the Red Goblin progression state reaches the required value.
Once quest zero is complete, the same authored branch introduces Malse as a
merchant, advances its persistent script flags, and then exposes message
`1000013`. Choosing `Trade` reaches sentence 113 and opcode 5. The resulting
left merchant panel and right inventory remain live together: stock can be
carried into the backpack or equipment slots, player items can be sold back,
gold is debited only after a purchase lands in an owned container, and Escape
returns an unfinished purchase to its original merchant owner. Purchase hover
text uses `Price`; owned-item hover text keeps `Sale Price`.

Syria's new-game status follows messages `1000040` and `1000041`. Its callback
starts quest zero with opcode 62 and selects that quest's notice with opcode
48 before waiting for the second bubble to close. Message events carry the
script character number, so the renderer can anchor Syria's bubble even
though this particular branch does not run the explicit actor-facing command
used by Ostare and Malse. Dialogue text, actor IDs, branches, and quest IDs
continue to come from the retail SCS.

The corresponding completion is authored on the outdoor map rather than in a
hard-coded enemy-name check. Red Goblin character `14010000` has status kind
`4` at sentence 12. After its death animation and fade finish, the retail
enemy owner invokes that status; its opcode 62 command changes quest zero from
state one to state two and emits the completion cue.

This is intentionally a narrow vertical slice. The messages use the
actor-anchored retail speech frame from `Hukidasi.njp`: its size comes from
the Shift-JIS-aware 6-by-12 `Font01.njp` text layout, and the tail follows
Ostare's live screen position. Ground-item state is created faithfully. The
executable-owned `Item.Ibn` loader keeps the inventory icon fields separate
from the ground resource and CAF chart fields. The four drops load their real
`Character/ITEM` animation and SDW shadow and share the normal depth-sorted
world pass with actors and scenery. Their CAF-selected palettes and
`Item.Ibn` RGB strengths also reproduce the default ground colors. They now
participate in opaque-pixel pointer selection and the common interaction-range
approach too. A successful pickup moves the concrete category, definition,
and quantity into `PlayerInventory`; that owner now feeds the live 9-by-4
inventory panel and retail save stream.

## How to extend it

Each new script slice should start with a real retail interaction:

1. identify the status trigger and starting sentence;
2. trace every sentence and opcode that the path can reach;
3. compare the command handlers and operand accessors with the executable;
4. implement only the missing general behavior, in the correct boundary;
5. preserve waits and asynchronous actor actions;
6. test the path with the original SCS data;
7. update this document with the new evidence.

Script-owned dialogue, quest branches, rewards, actor IDs, and service behavior
must not be copied into `WorldScene`, a renderer, or a state class. Native game
services should be small general hooks, and reusable behavior derived from a
DLL must remain under its matching `SF_EXE/libs/` directory.

The same rule applies to AI: actor decision tables belong to the future
portable `RKC_RPG_AICONTROL` boundary, while their native action handlers
belong to the relevant executable-owned actor or combat system. MCT-owned
placement and movement bounds should remain scenario data.

## Still to map

The next useful script work should keep following real scenario interactions
and combat rather than adding commands in isolation. That should naturally
reveal more of:

- persistent flag initialization and save-game restoration;
- message control bytes, portraits, speaker metadata, and alternate message
  frame modes;
- keyboard movement between message choices;
- waits for movement and animation completion;
- later warps, shops, rewards, and quest-log actions;
- remaining operand domains and the rest of the opcode switch;
- multiplayer and network flag behavior.

The goal is not merely to make Remote Town look scripted. It is to recover a
general interpreter that can eventually run all of Remote Town, then every
scenario in the original game from its own data.
