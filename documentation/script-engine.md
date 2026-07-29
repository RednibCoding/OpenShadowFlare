<!-- SPDX-License-Identifier: CC-BY-SA-4.0 -->
<!-- SPDX-FileCopyrightText: 2026 Michael Binder and OpenShadowFlare contributors -->

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

More status kinds exist and their event meanings still need to be named as we
reach them.

## Interpreter architecture

The portable implementation lives in
`src/SF_EXE/libs/RKC_RPG_SCRIPT/`. It is a separate static library with two
parts:

- `ScriptData` owns the decoded, immutable SCS data and its lookup helpers;
- `Interpreter` owns execution state, temporary flags, nested sentence frames,
  and message waits.

The interpreter deliberately does not include world, renderer, audio, UI, or
save-game headers. It asks the executable for those services through small
hooks:

- read or write an external operand domain;
- perform a native actor/game command;
- answer a typed query about game-owned state;
- present a decoded message.

This keeps the old DLL boundary visible without pretending that the original
DLL contained the whole game. It also means the interpreter can be tested with
the retail SCS file without creating a window.

Sentence calls use an explicit frame stack. A message command yields execution
and returns a wait state. Confirming the message resumes at the following
command rather than running the entire interaction in one frame. Unknown
opcodes return an error with their opcode and sentence context; they are never
treated as successful no-ops.

## Commands implemented so far

The retail opcode switch begins at `0x00430f80` and covers opcode values
`0x00` through `0x4b`. Only commands reached by the first vertical slice are
portable so far.

| Opcode | Retail address | Current meaning |
|---:|---:|---|
| 0 | `0x00431005` | Compare two evaluated operands and call a sentence when true |
| 1 | `0x004310a2` | Evaluate an operand and assign it to another operand |
| 2 | `0x00431294` | Resolve a message, present it, and wait for confirmation |
| 18 | `0x00431efa` | Native actor action used by the opening interaction |
| 21 | `0x00432094` | Native actor action with an evaluated value |
| 61 | `0x00433f16` | Write the local player's level to an operand |

Opcode 0 stores its comparison selector as a raw operand. The selectors seen
in the executable are:

| Selector | Test |
|---:|---|
| 0 | equal |
| 1 | not equal |
| 2 | greater than |
| 3 | less than |

The native meanings of opcodes 18 and 21 are not fully named yet. The first
conversation proves that they address an actor and put it into the interaction
state. Both evaluate their actor operand through the retail operand reader;
opcode 21 evaluates its additional value as well. Their hook remains generic
until more scripts reveal the complete behavior.

Opcode 61 is much narrower. The retail handler gets the local player, reads
the level field at offset `0x34`, and passes it to the common operand writer.
The portable interpreter asks its host for
`ValueQuery::local_player_level`, so player data stays game-owned rather than
being copied into the script library.

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
| 10, 11, 12 | Persistent global arrays |
| 13 | Local-player array |

Several additional types address actors and broader game state. They will be
named only when an exercised retail path gives us enough evidence.

Temporary flags are owned directly by the interpreter and initialized from the
SCS definitions. Persistent and game-owned domains are callbacks because their
lifetime belongs to scenario state, save data, the player, or another portable
DLL boundary. The initial world slice stores unknown-but-valid external values
in a generic keyed map. That is sufficient to preserve assignments while the
proper save and quest-state owners are reconstructed, but it is not the final
persistence model.

## First working conversation

The first end-to-end slice is Ostare's opening Remote Town interaction:

1. `Scenario.Mct` creates Ostare as local person zero.
2. A click on his rendered actor derives script character `12000000`.
3. Status kind `0` resolves to sentence `4`.
4. The interpreter follows its nested comparisons and sentence calls.
5. Native actor commands stop Ostare's wandering and turn him toward the
   player.
6. Message `1000000` is decoded from the retail SCS and shown.
7. Return or another click dismisses the message and resumes the sentence.
8. When execution completes, normal movement and Ostare's actor update resume.

The visible text begins with Ostare introducing himself as commander of the
area. That text is read at runtime from the original data; it is not present in
the OpenShadowFlare source.

Clicking Ostare again follows the next real SCS branch. Sentence six uses
opcode 61 to read the new character's level, selects the under-level-five
path, and shows message `1000005`. The interpreter retains the earlier
persistent assignment, so this is a continuation of the same script state
rather than a second hand-written interaction.

This is intentionally a narrow vertical slice. The message now uses the
actor-anchored retail speech frame from `Hukidasi.njp`: its size comes from
the Shift-JIS-aware 6-by-12 `Font01.njp` text layout, and the tail follows
Ostare while he moves. The rest of the opening quest chain will exercise more
commands and operand domains.

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

## Still to map

The next useful work is the rest of Ostare's opening conversation and quest
setup. That should naturally reveal more of:

- persistent flag initialization and save-game restoration;
- message control bytes, portraits, speaker metadata, and alternate message
  frame modes;
- choices and branching;
- waits for movement and animation completion;
- interaction range and the retail auto-approach behavior;
- gates, warps, shops, inventory, rewards, and quest-log actions;
- remaining operand domains and the rest of the opcode switch;
- multiplayer and network flag behavior.

The goal is not merely to make Remote Town look scripted. It is to recover a
general interpreter that can eventually run all of Remote Town, then every
scenario in the original game from its own data.
