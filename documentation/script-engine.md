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
shape appears throughout the scenario scripts for all 51 transport rows.

The rest of that loop is presentation. While overlapping, it raises temporary
flag `1000039` by 50 per update, plays sample 80 once through latch
`1000040`, draws message `1000060` (`Remote Town`) above object `10000200`,
and writes the rising value to objects `10000203` and `10000204`. Leaving
subtracts 50 per update, resets the sound latch, removes the label, and asks
the executable to close transport service zero. The label and both fades are
therefore SCS-authored behavior, not properties hardcoded onto a teleporter
class.

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
`0x00` through `0x4b`. Commands are made portable as real interactions and
services exercise them; unknown values still fail loudly.

| Opcode | Retail address | Current meaning |
|---:|---:|---|
| 0 | `0x00431005` | Compare two evaluated operands and call a sentence when true |
| 1 | `0x004310a2` | Evaluate an operand and assign it to another operand |
| 2 | `0x00431294` | Present a message, finish immediate sentence work, then wait |
| 4 | `0x00432296` | Mark every equipped, backpack, and belt item as identified |
| 5 | `0x0043222b` | Request one of the executable's numbered vendor inventories |
| 6 | `0x004321e8` | Rebuild a numbered vendor inventory from a Table 32 stock profile |
| 7 | `0x0043244d` | Fully restore the local hero and a living owned companion |
| 8 | `0x004324cf` | Fully restore the local hero's mana |
| 9 | `0x0043234a` | Repair one equipped item group, or the non-equipped backpack group for selector `-1` |
| 10 | `0x00431ca1` | Ask the world to create an item at evaluated coordinates |
| 11 | `0x00431ac5` | Add an evaluated value to a writable operand |
| 12 | `0x00431b0c` | Subtract an evaluated value from a writable operand |
| 13 | `0x00431b53` | Multiply a writable operand by an evaluated value with 32-bit wrapping |
| 14 | `0x00431b9b` | Divide a writable operand by an evaluated signed divisor |
| 15 | `0x00431bef` | Store the signed remainder from an evaluated divisor |
| 16 | `0x00417260` | Ask the world to play an authored sample, optionally range-limited at its evaluated position |
| 17 | `0x00432162` | Queue travel to an evaluated scenario and entry |
| 18 | `0x00431efa` | Stop a PEOPLE actor and enter its interaction state |
| 19 | `0x00431f72` | Native actor action which releases Ostare's interaction |
| 20 | `0x00431fc9` | Start an evaluated PEOPLE animation action with one-shot or repeated frame bounds |
| 21 | `0x00432094` | Turn a PEOPLE actor toward an evaluated target when its MCT flag allows it |
| 22 | opcode switch | Enable all three state channels for a scenario entity |
| 23 | opcode switch | Disable all three state channels for a scenario entity |
| 24 | `0x00417550` | Ask the world to create authored loot at evaluated coordinates |
| 25 | `0x004326c9` | Reactivate an inactive scenario-enemy slot at an evaluated position and direction |
| 26 | `0x00432b02` | Draw one evaluated decimal value above a player or scenario actor |
| 27 | `0x00432d05` | Draw an actor-anchored script message with evaluated offsets, color, and backing opacity |
| 28 | `0x00433022` | Run the target character's status-kind-six sentence inline when one exists |
| 29 | `0x00433056` | Send the network-client form of an unlock-switch notification; a single-player run has no local mutation |
| 30 | `0x0043309b` | Build a combat packet and submit an authored effect from an explicit projected origin |
| 31 | `0x00432762` | Search an inclusive enemy-character range for the first registered active entry and write its absolute character number, or `-1` |
| 32 | `0x004327c9` | Search an inclusive enemy-character range for the first registered inactive entry and write its absolute character number, or `-1` |
| 33 | `0x0043288d` | Find the nearest living local player inside an evaluated distance range and write that slot and world position |
| 34 | `0x004337b5` | Measure the judgement-bound distance from the local hero to a script character and write the result |
| 35 | `0x00432831` | Convert an evaluated world vector to the executable's truncated direction in degrees |
| 36 | `0x0043332d` | Submit a packetless one-pass visual at an evaluated world position |
| 37 | `0x004334da` | Request the transport service selected by the command argument |
| 38 | `0x00433544` | Close the matching script-opened transport service |
| 39 | `0x00431c43` | Write a random integer between two evaluated inclusive bounds |
| 40 | `0x00433409` | Submit a packetless one-pass visual attached to an evaluated player or scenario actor |
| 41 | `0x004335ac` | Toggle an executable-owned item service; zero selects Warehouse/Special Item and nonzero selects Giant Warehouse |
| 42 | opcode switch | Write the local player's current and maximum life to two operands |
| 43 | opcode switch | Write the local player's current and maximum mana to two operands |
| 44 | `0x00433692` | Write the local player's saved companion type to an operand |
| 45 | `0x004336a9` | Switch the local player's owned companion to an evaluated Table 60 row |
| 46 | `0x004336e0` | Write an evaluated draw strength to a type-zero scenario object |
| 48 | `0x00433868` | Select a quest notice and set its counter to 600 |
| 49 | `0x0043389b` | Retain one raw scenario message in the executable's map-caption buffer |
| 50 | `0x004321cb` | Write the current scenario-entry value to an operand |
| 51 | `0x00432fed` | Install the twenty evaluated integer substitutions used by later message `%d` fields |
| 52 | `0x004310d7` | Write the repair price for an evaluated equipment/backpack selector to an operand |
| 53 | `0x00433923` | Write the local player's total Gold to an operand |
| 54 | `0x00433940` | Spend an evaluated amount of Gold from the backpack owner |
| 55 | `0x0043397d` | Write whether any equipped, backpack, or belt item is unidentified |
| 56 | `0x00433a78` | Override one scenario entity's effective visible, pointer, and judgement states |
| 57 | `0x00433b1f` | Write the local player's stored gender value without remapping it |
| 58 | `0x00433b33` | Search the four automatic-item pages, backpack, and active equipment, then write zero or one |
| 59 | `0x00433ced` | Remove the first matching item from that same retail owner order |
| 60 | `0x00433edf` | Refresh the local player's one-update `UnlockSW` presentation marker |
| 61 | `0x00433f16` | Write the local player's level to an operand |
| 62 | `0x00433f29` | Update a quest's state and trigger its update/completion cue |
| 63 | opcode switch | Write the local player's current and maximum optional condition to two operands |
| 64 | `0x00434001` | Open an authored full-screen Epilogue or `VisualNN` presentation |
| 65 | `0x0043403e` | Refresh the colored falling-streak emitter with evaluated RGB and count values |
| 66 | `0x00433682` | Write the current local-player slot number |
| 67 | `0x004340e7` | Mark one spell as permanently learned in the player's saved magic owner |
| 68 | `0x004342de` | Award a percentage of the current level's experience threshold and run the ordinary level-up path |
| 69 | `0x0043412b` | Write whether one spell has the exact learned availability state |
| 70 | `0x00434186` | Map the local player's saved job to the occupation-menu selection and write it |
| 71 | `0x004341da` | Change the local player's saved job from an evaluated occupation-menu selection |
| 73 | `0x004343b0` | Close the ordinary gameplay panels and request the executable-owned Blackjack service |
| 74 | `0x00434412` | Write the most recent Blackjack result to an operand |
| 75 | `0x0043443c` | Create a table-backed item and place it in its authored automatic-item page and cell when absent |

Opcode 0 stores its comparison selector as a raw operand. The selectors seen
in the executable are:

| Selector | Test |
|---:|---|
| 0 | equal |
| 1 | not equal |
| 2 | greater than |
| 3 | less than |

Opcodes 49 and 50 are part of map initialization rather than conversation UI.
The first copies the selected SCS message verbatim into the retail buffer at
`0x0048d5f8` and clears its companion value at `0x0048d5f4`. No code which
reads that buffer has been found in the executable, so the portable runtime
retains the ID and text for fidelity and inspection but does not invent a
visible area-name banner. Opcode 50 writes the entry value installed by the
scenario loader. Dusty Ruins uses it to choose `B1F` or `B2F`, including when
an authored transition changes floors without changing the scenario ID.

Retail installs the local player and entry before running scenario status kind
`7`. It runs that status after both a changed-map load and a same-scenario
relocation. The portable transaction now follows that order, which also lets
initialization scripts safely query player level before building vendor stock.

Opcodes 57 and 66 expose two different parts of that installed player. The
handler at `0x00433b1f` resolves the current player and copies runtime field
`+0x28`, which is the saved gender at record offset `+0x18`; it keeps retail's
raw `0 = female`, `1 = male` representation. The shorter handler at
`0x00433682` calls `0x00434cd0` and copies the player-list current-slot field
at `+0x08`, so multiplayer-aware scripts receive the actual local slot from
zero through three rather than a made-up character number.

The shipped catalog contains ten opcode-57/66 pairs in ten scenarios. Every
command has one writable temporary-flag operand, and every sentence which
uses one query uses the other exactly once. Dusty Ruins (`00010000`) is the
first visible example: entry zero initializes a 200-update line, opcode 66
anchors it to the local hero, and opcode 57 selects message `1000004` for a
woman or `1000003` for a man. The ordinary status-kind-5 update, opcode 27
label owner, player record, and scenario slot remain separate owners; the
script library only performs the two queries and writes their destinations.

Opcode 56 evaluates a character number followed by visible, pointer, and
judgement values. The handler at `0x00433a78` finds the live entity and writes
an enabled flag plus those three overrides at runtime offsets `+0xfc` through
`+0x108`. It does not rewrite the three ordinary script variables. Type-zero
object drawing at `0x0045ddd0` uses the effective visible value, while
`0x0045e080` uses the effective pointer and judgement values. Near Remote Town
uses the command every update to swap objects 1030 and 1031 according to saved
script flag 71. Every shipped opcode-56 target is a type-zero object, though
the portable state owner keeps the same override available to every scenario
entity class.

Opcode 39 evaluates its lower bound first and its upper bound second, takes
exactly one value from the executable's shared Visual C++ random stream, and
writes `lower + rand() % (upper - lower + 1)` to its third operand. Both ends
are therefore possible. The shipped scripts contain 611 calls across 55
scenarios, and every call has exactly three operands. Of those, 285 choose
between zero and one and 41 choose from 20 through 40; spawn setup uses the
remaining calls with script-calculated upper bounds. The portable
script library asks its host for the next random value, keeping the DLL
boundary free of world ownership while still sharing the world's retail
random sequence.

Opcode 33 resolves its first operand through the scenario-character registry,
then searches the four local-player slots in numerical order. A candidate must
be active, alive, and in the same scenario. Distance comes from the shared
judgement-rectangle measurement; `-1` leaves either end open and every other
bound is inclusive. The nearest candidate wins and the earlier slot wins a
tie. Success writes the player slot followed by world X and Y. A valid source
with no candidate writes only `-1` to the slot, leaving both coordinates
alone; a missing source leaves all three outputs alone. The portable runtime
currently owns one local player, but the script-library boundary already
returns the complete target record so multiplayer can extend the world query
without changing the interpreter.

Opcode 35 evaluates X and Y, calculates `atan2(-Y, X)`, multiplies by the
executable's slightly truncated `57.29579143313326` degrees-per-radian
constant, and truncates toward zero. It does not normalize negative angles.
The shipped data contains 126 opcode-33 calls in 25 scenarios and 80
opcode-35 calls in 17 scenarios. Six target queries and one direction query
deliberately put a literal in the last output position, which the ordinary
operand writer ignores just as it does for other non-writable operands.

Opcodes 13, 14, and 15 continue the writable arithmetic group started by add
and subtract. All three evaluate the destination value before the right-hand
operand and write back through the common operand owner. Multiply keeps the
low 32 bits of the x86 `imul`. Divide and remainder use signed `idiv`, so the
quotient truncates toward zero and the remainder keeps the dividend's sign.
A zero divisor returns successfully without changing the destination. The
retail scripts contain 67 multiplies, 126 divides, and 195 remainders across
34, 45, and 27 scenarios respectively; every destination is a temporary flag.

Opcode 30 is the script-facing form of the executable's normal effect request,
not a new scenario-actor class. All 411 shipped calls have fourteen operands,
spread across 33 scenarios. The first two supply an origin, operand three is a
direction in degrees, and operand seven projects that origin along the retail
sine/cosine path. The remaining values fill the effect number, speed, height,
direction and selected words of the ordinary 77-word combat packet. One
shared random draw selects packet presentation `21000..21003` for nonzero
effects or `21007..21009` for effect zero. Near Remote Town's first periodic
spawn sentence uses the position of object `10055000`, submits effect two,
then follows it with a separately authored positional sound. The script
library only evaluates the fourteen operands; the world owns packet
construction, the random stream, and the effect runtime.

Opcode 36 is the lighter packetless version used for placed scenery effects.
Its seven operands are effect number, world X, world Y, display height,
direction, judgement right, and judgement bottom. A negative direction becomes
direction eight. The request has owner kind zero, an explicit origin, zero
left/top bounds, no combat packet, and the same common constructor value 200.
The one-pass owner adds one to the supplied lower-right coordinates and uses
that result for all four edges of its point judgement rectangle.

The shipped scripts contain 353 calls across 26 scenarios. Effects 20007 and
20008 occur 34 times each and select OPTION resources 11000005 and 11000006;
their omitted presentation values evaluate to height zero, direction eight,
and zero-sized bounds. Effect 20009 occurs 285 times, selects resource
11000007, uses height 150, and supplies directions one, three, five, or seven.
Near Remote Town's sentence 18 is the first direct fixture, and its periodic
status creates the six live visuals authored for that update.

Opcode 40 is the actor-attached form. Its two operands are the effect number
and source character. Player slots zero through three use owner kind one;
every other resolved scenario character uses owner kind four. The executable
copies the source judgement rectangle into the common request, leaves the
origin implicit so the one-pass owner resolves the actor position, uses
direction eight, and carries no combat packet. A missing source is a
successful no-op.

All 54 shipped calls have two literal operands and occur across 45 scenarios.
Eight use effect 20010/resource 11000008, while the other 46 use effect
20018/resource 10000020. The portable interpreter only evaluates and forwards
the pair. Actor lookup, geometry, effect resources, and presentation stay in
the world owner.

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

Opcode 27 takes eight operands: actor, X offset, Y offset, message ID, red,
green, blue, and black-backing opacity. The executable projects the actor,
measures Shift-JIS text on a 6-by-12 grid, horizontally centers it, and
bottom-aligns it at the evaluated offset. It submits a black rectangle with a
three-pixel margin, a black text pass at `+1,+1`, and then the colored text.
Remote Town uses `{10000200, 0, -160, 1000060, 224, 224, 224, 1000}`.

Opcode 46 evaluates an object character and strength, then writes the live
type-zero draw-strength field used by both static and animated rendering.
Remote Town feeds its 0-to-1000 pulse into objects `10000203` and `10000204`;
an opacity of zero also stops their hidden CAF update. Opcode 38 is the paired
service cleanup. It closes transport only when its argument matches the
script-opened selector, leaving an independently open right-side inventory
and its camera layout untouched.

The one shipped nonzero opcode-41 call is scenario `99000013`, sentence 10.
Its object `10000900` is named `Giant Warehouse` in the Tower of Ordeal 12F
MCT and passes argument one. The executable uses that value to select a
different character-owned service with ten 9-by-10 pages. This remains a
native request: the script chooses the service, while item ownership, page
input, rendering, and persistence stay outside the interpreter.

Opcodes 58, 59, and 75 expose a different item owner. Category-four
`Item.Ibn` records can carry a page number and fixed grid cell. A non-negative
page routes the item into one of four private player collections instead of
the backpack; these are not the `X` Warehouse or any Giant Warehouse page.
Opcode 75 creates the ordinary retail instance, refuses a duplicate in its
authored page, and inserts it at that fixed cell. Opcode 58 searches all four
pages first, then the backpack, active main hand, body, active off hand, head,
legs, and four accessories. Opcode 59 removes in precisely that order and
refreshes the player profile when it removes equipped gear. The belt,
alternate weapon set, Warehouse, and Giant Warehouse are deliberately not
part of either command.

The shipped scripts use these commands for story objects such as Malse's Gem
and Syria's Spirit Stone, the later orb and card sets, companion stones, and
ordinary category-three or equipment checks. This is why the interpreter
only evaluates operands and returns the query result: item construction,
owner order, equipment refresh, and persistence remain executable hooks.

Opcode 68 is the matching scripted reward path. Its argument is a percentage,
not a raw experience amount. The executable reads Table 13 at `level - 1`,
multiplies that threshold by the evaluated percentage with a signed 64-bit
intermediate, divides by 100, and adds the result to player-record experience.
Crossing the threshold invokes the same growth, full life/mana restoration,
900-update notice, and samples used by combat experience. Scenario `04900001`
sentence 30 demonstrates the real sequence: opcode 75 grants the Spirit Stone,
opcode 68 grants 50 percent, and opcode 16 plays the authored reward sample.

Opcodes 67 and 69 use the saved magic owner directly. Opcode 67 writes the
exact learned value `3` to the evaluated spell slot at player `+0x1440`.
Opcode 69 compares that same stored value with `3` and writes zero or one to
its second operand. It deliberately ignores the runtime-only All Spells debug
override. Shipped scenario `04100000` uses opcode 69 to branch on a reward
spell and later opcode 67 to grant it; the normal magic save block preserves
the result.

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

Opcodes 71 and 70 do the same for occupation changes. The saved runtime job
values are `16` for Mercenary, `6` for Warrior, `5` for Hunter, and `9` for
Wizard or Witch. Opcode 71 maps those to menu values zero through three;
opcode 70 accepts only selections one through three and writes Warrior,
Hunter, or spellcaster respectively. Invalid selections leave the record
unchanged. Scenario `03900003` contains the one shipped query/change pair:
its authored service remembers the current selection, changes the job after
the player's choice, and plays the accompanying sound through opcode 16.

Opcode 72 has no operands. Its three shipped call sites (`01000000` sentence
212, `02100000` sentence 357, and `03900002` sentence 102) all sit behind an
equipment-color conversation. The interpreter asks the world for that
service; the UI owner then snapshots the equipped weapon, shield, and body
color fields before opening the panel. This keeps the script library unaware
of item records, rendering, and input.

Opcodes 73 and 74 form a complete asynchronous service boundary. Opcode 73
has no operands. It closes the ordinary gameplay panels and asks the
executable to run Blackjack; cards, input, timing, audio, and drawing do not
belong to the interpreter. When the result display closes, the executable
runs scenario status kind `8`. The following sentence uses opcode 74 to write
the saved result: zero is a draw, one is a player win, and two is a dealer
win.

The shipped paths are Tower of Ordeal scenarios `99000018` and `99000023`.
Scenario `99000018` starts the game in sentence 22 and handles the result in
sentence 31. Scenario `99000023` continues through sentence 35. The portable
tests load the original SCS and exercise that launch/result pair, so these
sentence numbers and branches remain scenario data rather than C++ rules.

Opcodes 22 and 23 take a script character number and write one or zero to all
three of its live entity-state keys. Opcode 44 reads player-record offset
`0x140`, which is the currently owned companion type, through a typed host
query. Remote Town combines those commands with the play-mode operand to keep
the selected companion from also appearing as a clickable town NPC.

Opcode 45 is the matching mutation. It evaluates one Table 60 row, stores the
active dog's level and experience in the player's per-companion arrays, then
loads the selected row's saved values and clears its defeated countdown.
`0x00450500` destroys and recreates character `16000000 + player slot` at the
hero with full life, so a swap does not reuse the town PEOPLE actor or retain
the previous dog's presentation state. The six shipped calls cover companion
types zero through five across scenarios `00000000`, `01000000`, and
`03900005`. Remote Town's four `Swap Dogs` choices are therefore ordinary SCS
branches; the portable world does not contain a name-based companion menu.

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

The wounded branch continues through the authored callback rather than a
conversation special case. Opcode 19 first releases Syria, opcode 20 starts
PEOPLE action four as a one-shot, opcodes 7 and 8 restore the party, and opcode
16 plays the sample stored in temporary flag `1000061`. Action four maps to
Syria's CAF chart three. The PEOPLE owner presents frame zero first, keeps the
last frame for one update, and then returns to idle chart zero. Opcode 7 copies
the hero's live derived maximum life into current life and does the same for
the owned companion only while that companion is alive; it deliberately does
not revive a defeated companion. Opcode 8 independently copies the live
derived maximum mana into current mana.

Opcode 20 evaluates six operands: actor, action, repeat selector, restart
frame, end frame, and one trailing value retained by the native call. PEOPLE
actions four through nineteen map to CAF charts three through eighteen. A
repeat selector of `-1` plays once. Other values repeat, using the authored
restart and end frames; `-1` means frame zero or the chart's last frame. This
is kept in the PEOPLE action controller, not in the interpreter, because CAF
ownership and actor update timing belong to the world.

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
ranges and write `-1` to operand one when they close. The portable interpreter
and speech-bubble layout preserve that split.
The native Remote Town fixture walks to Gravity, opens his retail message,
checks the initial red `QUIT` selection, moves the red highlight to
`Check Status`, and opens the generated status speech before closing it through
Gravity's authored status-one release branch. A second interaction hits the
rendered `QUIT` range and writes option three. Unselected ranges use the retail
gray, and leaving all ranges keeps the most recent selection. This covers the
actual world-to-render-to-interpreter path rather than only testing the marker
parser by itself.

Opcode 3 is the companion status message. Its first operand is a constant
companion type and its second is the writable close result. The handler at
`0x0043167d` reads that type's saved level, rebuilds the profile from Table 60
and table `800 + type`, and opens the resulting text through the same ordinary
actor speech owner as opcode 2. Closing the bubble writes `-1` to the result
and enters status kind one; the shipped companion sentences have already set
their branch flag to two, so that callback releases the actor.

There are only six opcode-3 calls in the shipped scripts, one for every
companion type across three scenarios. The text uses the active companion's
level and experience fields but the requested type's table-backed profile.
The cap is `min(player level / 3 + 2, 35)`, producing `Experience Limit` at a
non-final cap and `Experience Max` at 35. The executable also labels magical
hit rate as `M Defense` and physical defense as `M Evasion Rate`; the portable
formatter keeps that retail display bug.

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
| 3 | Scenario-enemy lifecycle by local enemy number |
| 4 | Scenario temporary flag |
| 5 | Network/state domain |
| 6 | Script character's current world X |
| 7 | Script character's current world Y |
| 8 | Current play mode (`0` single player, `1` client, `2` server) |
| 9 | Whether the idle local player is within interaction range of a displayed script character |
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

Type `3` is the direct form of the scenario-enemy registry used by opcodes 31
and 32. Retail adds `14000000` to the operand value, looks up that exact MCT
enemy slot, and returns its lifecycle value. A missing entry returns `-1`, not
zero. All 160 shipped type-three operands are reads in opcode-0 comparisons;
none are assignment destinations.

Type `9` is the gate used by the shipped unlock-switch interactions. Retail
first requires the local player to belong to the active scenario and be in
normal idle action one. It then finds the requested character in the live
object-display registry and compares the two judgement rectangles against the
player's 159-unit interaction range. Missing, hidden, busy, or distant actors
return zero. All 60 shipped uses are the second operand of opcode 1, spread
across 22 scenarios and the seven `10041000..10041006` switch actors.

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

That introduction also offers mission one through message `1000024`; the
quest ID is not attached to Malse in C++. Black Hammer in West Ruins scenario
`00000004` owns loot row 6. Its zero attempt count expands to the active-player
count, every one of its ten slots selects Table 31 row 400, and that row fixes
the result to category 4, definition `99000000`. Picking up the gem therefore
sends it to automatic-item page zero rather than the backpack. On the return
visit, sentence 37 finds it through opcode 58. Retail finishes the rest of
that sentence immediately after opening message `1000028`: opcode 59 removes
the gem and opcode 62 completes mission one and queues sample 66 before the
bubble is dismissed. The remaining three callbacks show messages `1000029`
through `1000031` and release Malse. Quest state, Malse's progress flags, and
the removed automatic item all survive the ordinary save path.

Malse's `Identify Items` choice is script-owned as well. Opcode 55 first scans
the five ordinary equipment slots, four accessory slots, backpack, and belt.
When there is something unknown, opcode 51 inserts the authored flat price of
100 into message `1000017`; `NO` starts selected. Confirming `YES` uses opcode
53 to check the backpack's Gold, shows message `1000015` when the player is
short, or spends exactly 100 through opcode 54 before opcode 4 identifies all
of those owners. Their raw instance words are updated with the live flag, so a
normal save keeps the result. If everything is already known, the script shows
message `1000018` without opening another panel or charging Gold. This is a
merchant service distinct from the Identify spell, which selects one backpack
item and trains the spell.

`Repair Items` continues through sentences 117 and 75 rather than opening a
separate panel. Opcode 52 queries selectors zero through four for Arms, Head
Armor, Body Armor, Shield, and Leg Armor; selector `-1` covers non-equipped
items in the backpack. The script sums the first five values for All Equipped,
inserts all seven prices into message `1000014`, and starts on `QUIT`.

The groups come from executable ownership, not NPC-specific code. Arms include
both active and alternate main-hand pointers, while Shield includes both
off-hand pointers. The other three selectors address their one equipped slot.
Non-Equipped scans only category-zero weapons and category-one armor in the
backpack; belt and accessory items are not repairable. A zero price shows `It
has been repaired`, insufficient Gold reuses message `1000015`, and a
successful choice runs opcode 9 before opcode 54 spends exactly the displayed
price. All Equipped invokes opcode 9 once for each of the five groups. Every
branch returns to the newly priced Repair menu until `QUIT` releases Malse.

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

Enemy groups have a second script-facing state which is separate from their
three visible, pointer, and judgement channels. Opcodes 31 and 32 evaluate an
inclusive pair of absolute `14000000`-series character numbers and scan the
scenario enemy registry in ascending order. Opcode 31 returns the first
registered entry whose lifecycle value is one; opcode 32 returns the first
whose value is zero. IDs which are not present in the current MCT are skipped,
including for opcode 32, and either command writes `-1` when no entry matches.

That lifecycle value follows the enemy owner rather than HP alone. Retail sets
it to one when an enemy is activated and leaves it set while a zero-life enemy
plays its death chart and 120-update fade. The death owner clears it only when
that presentation expires, immediately before the matching status-kind-four
callback. The portable world exposes the same lifecycle through a narrow
interpreter hook, so group-clear and later encounter scripts do not need to
know about `EnemyActor` or duplicate combat state.

Episode 1's Dusty Ruins assignment is the first complete group-clear path.
After quest zero is complete, Ostare still waits until the saved hero reaches
level 30. His Remote Town branch then shows messages `1000007` through
`1000009`, starts mission three, selects its 600-update notice, and queues
sample 65. In Dusty Ruins scenario `00010004`, a periodic sentence scans
enemy characters `14000000` through `14000007` with opcode 31. Each defeated
Garam Goblin remains active until its death chart and fade expire, so the
mission cannot complete early. Once the scan returns `-1`, the script applies
its three object-state commands, plays its positioned sample, and completes
mission three with sample 66. Back in town, Ostare creates the Table 30 row-4
reward before message `1000011`, sets persistent flag two, follows with the
Cold Svalt message `1000012`, and never creates that reward again after a
save/load round trip.

Syria's neighboring Spirit Stone mission is separate from her healing path.
Once mission three is active, message `1000044` starts mission two and its
notice. Stone Spike in continued Dusty Ruins scenario `00010005` owns Table
30 row 23; all ten choices lead through Table 31 row 401 to fixed category 4,
definition `99000001`. This is Syria's page-zero item at cell `(1,0)`, not the
different page-two item with the same display name used by a later reward.
On return, message `1000045` is followed immediately by opcodes 59 and 62, so
the stone is removed and sample 66 is queued before acknowledgement. Its
callback opens message `1000046` and creates the fixed category-2 definition
`1100001` reward. Saving the completed state keeps the stone absent and sends
later visits back through Syria's ordinary recovery branch.

Completing Dusty Ruins also unlocks two independent Remote Town callbacks.
Malse requires mission three complete, Ostare's reward latch set, and his own
flag eight clear. Messages `1000025`, `1000026`, and `1000027` run across the
status-zero/status-one callback chain; only the last one calls opcode 10 for
category two, definition `1100000`. Syria checks the same completed mission
and Ostare latch but owns flag seven instead. Message `1000042` sets that
latch, and callback message `1000043` creates definition `1100002`. Both
commands use the NPC position plus the authored 200-unit offset and `-1`
spread values, so they remain ordinary airborne ground items with the
category-two sample 93 landing sound. Saving flags seven and eight prevents
the gifts from being produced again.

Ostare's Cold Svalt direction is backed by ordinary status-kind-three map
edges. The Episode 1 route is scenario 1 (`Near the Remote Town`) object 6 to
scenario 3 entry 1, scenario 3 object 0 to scenario 5 entry 0, and scenario 5
object 1 to scenario 6 entry 1. In `Wasteland of Pillars`, object 3 enters
sentence 9. The sentence compares mission three with completed state two and
only then reaches sentence 10's opcode 17 call for scenario `1000001`, entry
zero. The incomplete branch contains no travel command, so touching the same
edge before clearing Dusty Ruins correctly does nothing. Scenario `1000001`
is the enemy-occupied Cold Svalt Town map; it is distinct from the recovered
town in scenario `1000000`.

The next edge and first town mission stay script-owned as well. Object 2 in
occupied scenario `1000001` calls opcode 17 for inhabited Cold Svalt scenario
`1000000`, entry zero. Alex's clear flag 11 starts messages `1000000..1000006`
through his status-zero/status-one chain and saves the first-visit latch.
Rosanna owns flag 15: her first visit runs messages `1000047..1000049`, and
the next runs `1000050..1000051`, sets mission four active, selects its notice,
and queues sample 65. Wild Ice in the occupied outskirts owns loot row 56 and
therefore the fixed category-four definition `99000002`, stored on automatic
page zero at `(2,0)`. On return, Rosanna's message `1000053` is followed by
opcode 59 and opcode 62 before acknowledgement, so the ruby is removed and
sample 66 is queued immediately. Callback message `1000054` creates category
two definition `1100003`; later visits use message `1000055` without repeating
the item.

With mission four complete, Alex's next status chain runs messages
`1000009..1000012` and sets mission six active. Cold Ruins bottom-floor
scenario `1020002` periodically scans enemy characters `14000000..14000006`
with opcode 31. The two Frost Golems, four Knight Frost Goblins, and King Frost
Goblin must all finish their death presentations before the scan is empty.
Only then does the script hide object `10011000`, show `10011001` and
`10011002`, play its positioned sound, and complete mission six with sample
66. Alex's completed branch opens message `1000014` and creates category four,
definition zero with quantity 2,000: the normal Gold ground item and sample 85
landing sound. Its callback opens `1000015`, starts mission seven, and queues
sample 65; later visits use `1000016`, so saving and loading cannot repeat the
Gold or the handoff.

The Purgatory assignment uses the same generic pieces on different shipped
maps. Vaporous Forest scenario `1000002` object 2 calls opcode 17 for scenario
`1030000`, entry zero. After traversing that map, object 1 calls opcode 17 for
scenario `1030002`, also at entry zero. Its periodic sentence scans character
numbers `14000000..14000006`: three Arc Shamans and four Arc Thunder Bats.
When all seven death presentations expire, the script hides object `10011000`,
shows `10011001` and `10011002`, plays the room sound, and completes mission
seven with sample 66. Alex's `1000017` branch creates category four definition
zero with quantity 4,000, followed by sample 85 on landing. Callback messages
`1000018..1000020` start mission eight and sample 65; saved active state uses
`1000021` and does not create the Gold again.

Mission eight begins at Hanged Men's Forest scenario `1000003`, whose object
1 enters Remains scenario `1040000`. Object 1 there enters `1040001`, and
object 1 in that inner map reaches clear room `1040002`. The room scans enemy
characters `14000000..14000006`: two Earth Golems, two King Earth Goblins,
and three Arc Goblin Shamans. After every lifecycle clears, it swaps objects
`10011000..10011002` with sample 34, swaps `10021000..10021001` with sample
31, runs Table 30 loot row 63 at the second door, and completes mission eight
with sample 66. Alex's message `1000022` creates 6,000 Gold; callbacks
`1000023..1000024` start mission nine and sample 65. Saved active state uses
`1000025`, so neither Gold nor the handoff repeats.

Mission nine is a map-discovery assignment, not another group clear. Object 5
in Remains scenario `1040002` calls opcode 17 for Sea of Trees scenario
`1000004`, entry one. Object 0 there enters Immortal Remains scenario
`1050000`, entry zero. The destination's status-kind-seven initialization
calls opcode 62 immediately, completing mission nine and queuing sample 66 as
the map loads. Back in town, Alex's messages `1000026..1000028` start mission
ten and sample 65. There is deliberately no Gold or item command in this
handoff; saved active mission ten uses message `1000029`.

Mission ten continues through object one in Immortal Remains scenarios
`1050000` and `1050001`, then reaches the seven-Gargoyle room in `1050002`.
Its four ordinary Gargoyles use loot row 55 with a 50-percent Gold branch of
200..300; the three magic variants use the same loot row with guaranteed
600..800 Gold. The periodic sentence scans characters `14000000..14000006`
and waits until all seven death presentations have expired. It then hides
object `10011000`, shows `10011001` and `10011002`, plays positioned sample
34, and completes mission ten with sample 66.

Alex's completed branch opens message `1000030`, sets the episode-state flag
to two, and creates category-four definition zero with quantity 10,000. Its
normal landing path plays sample 85. Callback message `1000031` explains the
Tower of Ordeal, and the following callback runs opcode 64 with value zero,
which is the Episode 1 Epilogue presenter. After it closes, flag value two
sends later visits to message `1000033` and sets the separate one-time flag
71, pointing the player toward Mining Town without creating the reward again.
Both flags and the completed mission survive the retail save extension.

Opcode 25 is the other half of that lifecycle. Its four evaluated operands are
absolute enemy character number, world X, world Y, and direction. The native
owner refuses to mutate an already-living enemy, but the script command still
returns successfully. An inactive slot is moved without changing its authored
spawn rectangle, restored to maximum life, reset to idle action 7, made fully
opaque, and cleared of old AI, movement, reaction, damage-attribution, and
death state. The slot is not recreated: expired MCT enemies stay in their
original vector position so later script lookups keep stable identities.

Opcode 28 evaluates one character number and runs that character's status-kind
six sentence inline through the normal frame stack. Nested calls carry the
target character context, and returning restores the caller's context. A
target without kind six is a successful no-op. The shipped scripts contain 181
calls across 32 scenarios, and every target has a matching kind-six status.

Scenario `04000003` shows how the pieces fit. Its periodic sentence uses a
type-three read for controller enemy `14030000`, opcode 32 to choose an expired
slot from `14030001` through `14030005`, and opcode 28 to create effects 20007
and 20008 plus positional sample 27. Forty updates later, opcode 25 activates
the chosen slot at object `10030000` facing direction 7. Native coverage runs
this shipped controller end to end and proves the expired slot remains present,
shows the wave effect, and returns at full life.

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

## Scripted unlock-switch feedback

Switch progress is a script-owned interaction, not a hard-coded map feature.
The status-zero sentence first uses operand type `9` to confirm that the idle
hero has actually reached the displayed switch actor. The authored arithmetic
then computes the progress value before opcode 26 draws it as centered decimal
text at the actor, with a one-pixel black shadow and no rectangle behind it.
Its remaining operands provide the world offsets and RGB strengths.

Opcode 60 refreshes the player's transient switch marker for that update.
`0x00434ef0` draws `Player/Common/UnlockSW` as chart zero, direction eight,
using the player's action counter and full RGB strengths. The player update
clears the marker before each status-kind-five pass, so the periodic script
must keep refreshing it while the switch sequence is active.

Opcode 29 is the multiplayer counterpart. Its handler sends packet `0x22`
with event kind six only when the executable is a network client. The shipped
scripts select it behind a play-mode branch; local single player reaches
opcode 28 instead. The portable single-player host therefore accepts opcode
29 without inventing local state.

This family appears consistently in the shipped data: opcodes 26 and 60 each
have 60 calls across 22 scenarios, while opcode 29 has 61 calls across 23.
The scenario audit fixes their operand shapes and actor distribution so later
switches keep using the same general interpreter and world presentation.

## Scripted full-screen visuals and weather

Opcode 64 is the script owner behind the presenter at `0x00417bd0`. Its one
evaluated value selects `Waiting.njp` pattern 4 for value zero or
`Visual%02d.njp` for a nonzero value. The page starts fully dark, reaches full
strength over 120 presented frames, and does not accept an advance until frame
300. Return, Escape, and the primary mouse button all use the same advance
path. While a page owns the screen, retail freezes the current player action
in place rather than cancelling it; that action resumes when the presentation
ends. A resource with more than one pattern starts its next page at counter
one; the final acknowledgement releases the resource and returns control to
the world.

The shipped scripts call opcode 64 seven times across six scenarios, using
each value from zero through six exactly once. `Visual02` is the only shipped
two-page resource. These are story, briefing, and selection pages, not map
loading screens, so normal scenario transitions still show only the
crossed-swords loading artwork while work is actually pending.

Opcode 65 is a separate screen-space particle command. The first three
operands are RGB bytes and the fourth is the number of new streaks for that
update. Each streak consumes five values from the executable's shared random
stream for its X origin, speed, short line length, opacity, and angle. It
starts at Y `-30`, moves down with the retail trigonometric projection, draws
through the DIB-style line path, and expires when its leading point reaches Y
`479`. The command only refreshes one frame's spawn request; periodic status
kind five calls are what keep rain, snow, or colored ambience going.

There are 22 shipped opcode-65 calls across 21 scenarios. Twenty request five
literal streaks. The other two calculate a temporary count from distance.
The audited colors are eight red, twelve white, one pale red, and one blue.
The interpreter evaluates those operands, while the world owns particles and
GAPI owns line drawing.

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
