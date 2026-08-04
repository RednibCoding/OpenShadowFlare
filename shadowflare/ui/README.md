<!--
Copyright (C) 2026 Michael Binder and contributors

This file is part of OpenShadowFlare.

OpenShadowFlare is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

OpenShadowFlare is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for details.

You should have received a copy of the GNU General Public License along with
OpenShadowFlare. If not, see <https://www.gnu.org/licenses/>.
-->

# Game UI

This folder owns the visible game interface: the HUD, panels, buttons,
tooltips, conversation bubbles, nameplates, and overlays. UI code may turn game
state into drawing commands and input into simple player intent, but gameplay
rules stay in `game/` or `interpreter/`.

Screens compose these pieces and control their lifetime. The `render/` folder
only supplies reusable drawing primitives; UI layout and behavior never belong
there.

Conversation text layout, pointer resolution, and bubble composition are
separate on purpose. The layout code understands the retail Shift-JIS column
rules and hidden choice markers. The input code turns those rendered spans
into simple choice intent, and the bubble code only draws the result with the
original frame patterns. None of them advances scripts or changes actor
behavior.

The gameplay inventory follows the same split. Its draw files compose the
authored right Inventory and left Special Item frames, retained item cells,
and the pointer-held icon from the player owners. The input file owns the `I`,
`X`, ITEM-button, panel, backpack, Special Item, and Close rectangles, then
produces the shared world-view offset and simple take, place, or world-drop
intent. Inventory ownership, transactional swaps, drop placement, and the
until-release pointer guard remain in `game/`; no UI file talks to a target
backend.

Status and Magic are two tabs of one left-hand character panel owner. The
Status draw file composes retail pattern 5, identity and derived values, and
the affinity display. The game-side player profile owns the arithmetic; UI
code does not become the source of combat stats. The Magic draw file composes
the four spell pages, descriptions, panel bar, persistent HUD bar, and held
icon. Its input file owns the recovered page, icon, drag, and dynamic bar
rectangles and emits only selection/assignment intent. `game/player_magic.c`
remains the sole owner of saved spell state. `gameplay_panels_input.c`
coordinates both tabs with Special Item, the independent right Inventory
panel, Escape, the common camera offset, and click consumption.

The owned-companion strip follows the same rule. Its draw file composes the
retail life bar and active/inactive `Bar.njp` cells, while its input file owns
the exact bottom-left hit rectangle and emits only a toggle intent. Companion
life, follow behavior, collision, and activity state remain in `game/`; Space
and controller bindings stay at the platform-neutral runtime edge.
