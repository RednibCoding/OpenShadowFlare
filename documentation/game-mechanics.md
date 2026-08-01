# ShadowFlare Game Mechanics

## Character Classes

The game has 3 base playable character classes:

| Class   | Gender | Description |
|---------|--------|-------------|
| Hunter  | Both   | Ranged combat specialist (different attack revision) |
| Warrior | Both   | Melee combat specialist |
| Wizard  | Male   | Magic specialist |
| Witch   | Female | Magic specialist (female Wizard) |

The saved job field uses retail values `6` for Warrior, `5` for Hunter, and
`9` for either Wizard or Witch; gender chooses the displayed spellcaster
name. A new character begins as job `16`, displayed as Mercenary. Scenario
scripts use menu values zero through three instead: opcode 71 maps the saved
job to that menu value, while opcode 70 changes choices one through three
back to Warrior, Hunter, or spellcaster. Changing occupation does not rewrite
the character's earlier per-level job history.

### Equipment colors

Weapons, shields, and body armor can carry one of 16 alternate appearance
colors. `-1` means the item's authored default. The selected value changes the
RGB strengths of the item's primary character part; a weapon's secondary part
keeps its own authored strengths. The value is stored inside the normal item
instance, so moving, dropping, equipping, and saving an item all keep its
color.

### Class Advancement
Classes can be promoted to advanced classes:
- **Mercenary** - Advanced Warrior/Hunter

Class change messages:
- "It is possible to change your occupation to [%s]"
- "Changed your occupation to [%s]"

### Class-Specific Mechanics
- Hunters have different attack revision: "Revision of Attack (Not for Hunters):-%d%%"

## Gender

Players can choose:
- Male
- Female

Each gender has separate animation files in `Player\Male\` and `Player\Female\`.

## Elements

The game has 8 elemental types, each providing a stat bonus:

| Element | Stat Bonus |
|---------|------------|
| Fire    | Attack +% |
| Water   | Defense +% |
| Earth   | Hit Rate +% |
| Thunder | Evasion Rate +% |
| Holy    | Magical Attack +% |
| Dark    | Magical Defense +% |
| Gel     | Magical Hit Rate +% |
| Metal   | Magical Evasion Rate +% |

This was discovered at function 0x00405750, which displays:
- "Effect of Fire\n    Attack               %+d%%"
- "Effect of Water\n    Defense              %+d%%"
- etc.

### Status window

`S` opens Status on the left without pausing the world. Status and Magic are
two tabs of the same window, and either tab can remain open beside Inventory.
The Status tab shows the character's name, class, level, experience, current
and maximum HP/MP, weight, physical and magical combat values, movement and
attack speed, and the eight elemental affinities. Grey values match the saved
base stat, red values are lower, and gold values are higher. The elemental
diagram uses the saved x/y alignment point plus equipment and item bonuses.

## Spells/Skills

Complete spell list (from 0x00407a60 MagicWindowDisplay):

### Attack Spells (damage-based)
| Spell | Stat Display |
|-------|--------------|
| Fire Ball | Attack |
| Ice Bolt | Attack |
| Ice Blast | Attack |
| Hell Fire | Attack |
| Plasma | Attack |
| Lightning Storm | Attack |
| Flame Strike | Attack |
| Earth Spear | Attack |
| Sonic Blade | Attack |
| Dread Deathscythe | Attack |
| Medusa | Attack |
| Mud Javelin | Attack |
| Explosion | Attack |
| Elemental Strike | Attack |
| Counter Burst | Attack |

### Support/Utility Spells
| Spell | Stat Display | Description |
|-------|--------------|-------------|
| Heal | Heal % | HP restoration |
| Energy Shield | Shield % | Routes ordinary damage to MP and changes physical defense |
| Magic Shield | Def % | Reduces effect-family damage and charges MP on each shielded hit |
| Berserker | Attack % | Attack boost |
| Moon | (none) | Increases companion stats while active |
| Transport | (none) | Teleportation |
| Identify | (none) | Identify items |

### Spell Stats
Each spell has:
- **Lv** - Spell level
- **Exp** - Experience towards next level
- **MP** - Mana cost
- **Effect %** - Heal%, Def%, Shield%, Attack%, RefPer% (reflection percent)

### Magic window and bar

`M` opens the Magic window on the left without pausing gameplay. The camera
moves right so the player stays centered in the uncovered half; Inventory can
remain open on the other side.

The window shows six spells at a time across four pages. Availability value
`3` is a learned, usable spell, value `1` is shown dimly, and the other states
leave an empty slot. The large icon is `MagicIcon.njp` pattern `spell + 2`.
Each visible learned row shows its stored level and experience, Table 27's
next threshold, Table 16's MP cost, and the relevant Table 17 effect.
Hovering the spell name uses Table 600 through 621 for the help text.

Dragging either a learned page icon or an occupied window-bar slot onto one
of the eight slots assigns it there. A spell can only appear once, so moving
it clears the old slot first. Picking an icon plays sample 57; page changes
and successful bar changes play sample 58.

The small bar above the HUD uses `MagicBarIcon.njp`. It starts at x=224 in a
full world view, x=344 with a left panel, and x=124 with a right panel, adding
the retail four-pixel gap before slots one and five. The selected spell is
drawn as its larger MagicIcon at y=382; ordinary entries sit at y=392.
Clicking a learned entry selects it and clears normal-attack targeting.
Clicking the final targeting icon does the reverse.

### Fire Ball

With Fire Ball selected, right-clicking a pointed enemy consumes the command.
The spell must be learned and the hero must have enough MP; the MP cost comes
from Table 16 at the effective spell level, reduced by equipped item parameter
19 but never below one.

The hero faces the target and plays action 23, using CAF chart 13 for the cast
and chart 14 for recovery. The chart's `0x40` marker determines the delayed
effect launch. Effect 10001 then owns the visible projectile, launch and
impact sounds, collision, and damage instead of the input or animation code.
Spell experience is awarded when the family-zero spell packet successfully
reaches a target, not when the button is pressed, so a miss does not train the
spell. Table 27 supplies the next-level threshold, the cap is level 20, and
only one spell level may be gained from one contact.

### Ice Bolt

Ice Bolt uses the same pointed-enemy command and chart-13/chart-14 player
casting shell as Fire Ball, but its authored payload is separate. It enters
action 24, reads Table 20 row two, creates effect 10002, marks packet subtype
one, and requests impact presentation 21013. After the CAF marker delay,
effect 10002 places resource 10000040 180 world units in front of the hero,
launches it with 50-unit collision bounds, and plays sample 94. Impact still
uses sample 20 and awards Ice Bolt practice only when its packet reaches an
enemy.

### Plasma

Plasma is still aimed by right-clicking an enemy, but it is not a projectile.
The hero enters action 25 and uses CAF charts 11 and 12. Effect 10003 starts
from the hero's position and sends waves along the target angle at radii 250,
450, 650, and so on, with the count supplied by Table 205 at the effective
spell level.

Each clear wave creates three visual layers; the damaging layer uses a random
chart, checks every enemy in its 100-unit area on its first update, and plays
sample 21. If one wave position is blocked by the map or a solid scenario
object, that wave and every later wave are suppressed. Every enemy packet
contact can award Plasma practice through the ordinary receiver path.

### Hell Fire

Hell Fire uses the ground/self secondary-click path rather than requiring an
enemy under the pointer. The clicked point decides which way the hero faces,
but the spell itself erupts around the hero. It enters action 26, uses CAF
charts 13 and 14, consumes the Table 16 MP cost, and creates effect 10004 at
the chart's cast marker.

The effect first shows its warning layer. At the delayed burst it creates two
visual layers, plays samples 29 and 23, and applies the spell packet to every
valid target inside the hero judgement area expanded by 150 world units.
Contacts use sample 20, shake the camera briefly, and award Hell Fire practice
through the same receiver-owned path as the projectile and Plasma spells.

### Ice Blast

Ice Blast also uses the targetless secondary-click path. The hero faces the
clicked point, but the effect stays centered on the hero rather than being
placed at the cursor. Action 27 uses CAF charts 11 and 12 and creates effect
10005 with the Table 20 row-five timing and MP cost.

On update three the effect captures the hero's position and starts resource
10000051. Its authored frame count schedules the later 10000050 and 10000052
layers, the 150-unit expanded area contact, a short camera shake, and six
sample-22 pulses. Contact uses sample 20 and awards Ice Blast practice through
the normal receiver path.

### Heal

Heal is a self-cast on the targetless secondary-click path. It enters action
28 with CAF charts 11 and 12, but waits for the chart-11 `0x40` marker before
doing anything. At that marker it always shows effect 21020, which uses
resource 11000060 at the hero for one animation pass.

When HP is missing, Heal restores the Table 17 row-six percentage of maximum
HP, capped at the amount missing, plays sample 17, and awards one practice
point. Casting at full HP still consumes the normal Table 16 MP cost and shows
the visual, but does not play sample 17 or train the spell.

### Moon

Moon is a targetless self-cast that enters action 29 with CAF charts 11 and
12. The normal Table 16 MP cost is paid when the action begins. At chart 11's
`0x40` marker the spell toggles: an inactive Moon turns on at its current
effective level, while an active Moon turns off.

While it is active, Table 200 row zero is added to the hero's mental recovery
rate. Retail applies that rate to maximum MP every third game update and keeps
the sub-point remainder between updates. Moon turns itself off as soon as MP
reaches zero. This live toggle, its effective level, and its remainder are not
part of the character save record, though they remain active when moving from
one scenario to another during the same game.

Rows 1 through 13 of Table 200 modify the companion's attack speed, walking
speed, running speed, physical attack, maximum HP, hit rate, physical defense,
physical evasion, magical attack, magical hit rate, magical evasion, magical
defense, and parameter 17. Each value is a percentage of the companion's base
value at the Moon level. Speed values are clamped to 0..255; the remaining
combat values are kept at one or higher. Recomputing these bonuses preserves
the companion's current HP instead of treating the change as a heal.

Resource 11000040 loops at the companion while Moon is active. It is hidden
while the companion is dead, defeated, or reviving, and its animation counter
continues from where it stopped once the companion can be shown again. A kill
credited to the local player slot while Moon is active awards one point of
Moon practice. Retail uses the source character number modulo ten, so both the
hero and the owned companion can supply that kill.

### Berserker

Berserker is a targetless self-cast that enters action 30 with CAF charts 11
and 12 and Table 20 row eight. It pays the normal Table 16 MP cost when the
action begins, then toggles at chart 11's `0x40` marker. Its live effective
level and active state are not written to the character save.

Table 201 row zero is Berserker's continuing MP rate. Berserker and Moon do
not have separate drain clocks: retail adds both rates, applies the total to
maximum MP every third game update, and carries one shared fractional
remainder. Reaching zero MP disables both sustained spells and rebuilds the
affected player and companion values.

That same update also includes equipped rolled parameter 18 and a five-point
bonus from special item 98000004. Its life-side partner uses equipped rolled
parameter 17 or special item 98000003, keeps a separate remainder, and clamps
a living hero to at least one HP. Both rates use the same three-update cadence.

Rows 1 through 12 modify the player's attack speed, walking speed, maximum
HP, maximum MP, physical attack, physical defense, hit rate, physical
evasion, magical attack, magical defense, magical hit rate, and magical
evasion. The percentages are applied after ordinary equipment contributions.
Speeds are clamped to 0..255 and the remaining values to at least one. In the
shipped Table 201 the maximum-pool rows are zero; offense and speed rise while
physical defense and evasion fall. Integer percentage calculations truncate,
so a small base value does not necessarily gain a whole point.

While active, `Player/Common/Powerup.Caf` and `.Njp` loop over the hero using
chart zero, direction eight, and red/green/blue strengths 1000/200/200. Kills
credited to either the local hero or owned companion train Berserker through
the companion-spell practice mode.

### Energy Shield

Energy Shield is another targetless self-cast. It enters action 31, uses CAF
charts 11 and 12 with Table 20 row nine, and pays the normal Table 16 MP cost
when the action begins. The live shield toggles only when chart 11 crosses a
`0x40` marker. An inactive cast cannot turn it on if that up-front cost used
the player's last MP; an active cast still turns it off normally.

The shield does not have a separate hit-point pool or a Table 202. While it is
active, the Table 17 value for spell nine scales the physical-defense value
used for ordinary damage. That damage is then subtracted from MP instead of
HP. Damage beyond the remaining MP does not spill into HP: MP becomes zero,
and later ordinary hits reach HP. Effect-family damage bypasses Energy Shield
and continues to use HP.

Reaching zero MP turns the live shield off on the player update. Its active
state and animation frame are runtime-only and are not saved. The aura reuses
`Player/Common/Powerup.Caf` and `.Njp` after the Berserker pass, with chart
zero, direction eight, and strengths 1000/1000/300. Any kill credited to the
local hero slot while it is active trains spell nine, whether the hero or the
owned companion dealt the final blow.

### Magic Shield

Magic Shield is a targetless self-cast on action 40. It uses CAF charts 11
and 12 with Table 20 row eighteen, pays the ordinary Table 16 MP cost when
the command begins, and toggles its runtime flag at a newly crossed chart-11
`0x40` marker. Unlike Energy Shield, an exact-cost cast may turn Magic Shield
on for that marker update at zero MP; the next player update turns it off.
Casting it again while active pays the command cost again and toggles it off.

The shield only intercepts effect-family damage packets. Table 17 spell
eighteen parameter zero reduces the already-resolved damage, with a minimum
of one point. Every intercepted hit creates effect 21029, which is the
one-pass resource 11000241, and plays sample 60. Damage of at least 20 after
the reduction awards Magic Shield practice.

Each shielded hit also charges MP. Retail gets the base cost from the
currently selected magic row at Magic Shield's effective level, then subtracts
equipped instance parameter 19 and clamps the result to at least one. This
means changing the selected spell can change Magic Shield's hit cost. A hit
that empties MP disables the shield immediately; otherwise a zero-MP player
loses it at the start of the next player update.

Resource 11000240 loops at the hero with chart zero, direction eight, and
normal 1000/1000/1000 color strengths. Its active flag and animation frame
survive ordinary scenario travel during the same game, but are not written to
the retail character save and are cleared by death. Magic Shield and Counter
Burst are mutually exclusive in retail; their marker actions clear the other
live flag.

### Counter Burst

Counter Burst is the matching targetless toggle on action 41. It uses CAF
charts 11 and 12 with Table 20 row nineteen and pays the normal command-time
MP cost. Its chart-11 `0x40` marker toggles Counter Burst, resets its own aura
frame, and clears Magic Shield. An exact-cost activation can be visible for
that marker update before the next zero-MP player update turns it off.

The spell only reacts to a reflectable packet from a living enemy that still
exists in the current scenario. Its Table 17 parameter-zero percentage is
added to any successful equipment reflection percentage. The returned packet
uses the resolved incoming damage, applies the combined percentage, halves
the result for the retail source value 100, and keeps at least one point of
damage. Counter Burst uses effect 21030/resource 11000251 and sample 60; a
post-resolution incoming hit of at least 20 also trains spell nineteen.

Each successful Counter Burst reflection charges MP through the same retail
quirk as Magic Shield: parameter two comes from the currently selected magic
row at Counter Burst's effective level, equipped instance parameter 19 lowers
it, and the result cannot be less than one. A missing or dead source produces
no reflection, effect, practice, or MP charge. Emptying MP disables the spell
immediately.

Resource 11000250 loops at the hero using chart zero, direction eight, and
1000/1000/1000 color strengths. Retail draws it after Magic Shield and before
the Berserker and Energy Shield passes. The runtime flag and frame survive
ordinary scenario travel, are not saved to disk, and are cleared on death.

### Explosion

Explosion is a targetless ground command on action 42. It uses player CAF
charts 11 and 12, pays its normal MP cost when the command begins, and waits
for the chart-11 `0x40` marker. At that marker the clicked point must fit the
owned companion's whole collision rectangle. A missing, dead, special-action,
or blocked companion does not refund MP and does not create a fallback effect.
An ordinary attack or hit finishes first, then the queued Explosion command
takes over.

The companion plays PARTNER chart six in direction eight at its old position,
instantly relocates when chart seven begins, and plays chart seven at the new
position. The boundary submits sample 45 twice. Its chart-seven `0x40` marker
plays sample 46 and creates effect 21031: two overlapping resource-10000000
layers on charts one and zero with RGB 500/500/1200, samples 29 and 23, and
nearby camera shake. Living enemies whose bounds overlap the 640-by-640 blast
area roll the companion's hit rate against their physical evasion.

Explosion's damage packet has two intentional oddities from retail. The base
damage field is the owner hero's magical defense, while the table scaling
level comes from Elemental Strike (spell 21). Explosion itself still owns the
spell-20 parameter rows, randomized ordinary hit effect, and practice award.
After the arrival chart finishes, the companion unlocks and resumes normal
follow and combat AI.

### Earth Spear

Earth Spear returns to the pointed-enemy command path. It enters action 32 on
CAF charts 11 and 12, pays the normal Table 16 cost, and creates effect 10010
from the hero's cast-time position toward the selected enemy. The spell does
not launch a travelling projectile.

Its family-zero packet uses subtype three, the hero's physical defense and
magical hit rate, Table 17's Earth Spear values, packet flag 72 set to one,
and one of ordinary impact presentations 21000 through 21003. Successful
contacts train spell ten through the common receiver path.

Table 206 controls how many stone ridges are attempted. Starting 250 world
units from the cast origin, a new resource-10000060 ridge is placed every
eight updates with 300 units between ridges. Each clear ridge plays sample 22,
shakes a nearby camera briefly, and applies the packet to every valid enemy in
its 150-unit area. If the first placement is blocked by scenery, retail
suppresses it and every later ridge; after a ridge has appeared, a later
blocked placement only ends the remaining line.

### Flame Strike

Flame Strike is a pointed-enemy cast on action 33 and CAF charts 13 and 14.
It pays the normal MP cost and creates effect 10011 immediately, carrying the
selected enemy, the cast direction, the hero's judgement rectangle, Table 17
travel speed, and the chart marker as its launch delay.

Its family-zero subtype-zero packet uses magical attack, magical defense, and
magical hit rate with the spell's Table 17 values. Presentation 20000 is used
on contact, and the effective spell level is passed to the controller.

Resource 10000012 appears at the hero when the controller starts. At the
authored delay, Table 204 selects between two and eight resource-10000010
projectiles arranged around a full circle. They begin 180 world units from the
hero, turn toward the selected enemy in 20-degree steps, and live for at most
90 updates. Scenery or the first target ends an individual projectile. The
last projectile spawn plays sample 19; each contact uses sample 20 and trains
Flame Strike through the ordinary receiver path.

### Dread Deathscythe

Dread Deathscythe is another pointed-enemy spell, using action 34 and CAF
charts 13 and 14. Its effect-10012 request carries Table 17 travel speed,
display height 200, the selected target, the aim direction, the hero judgement
rectangle, the marker delay, effective level, and constructor field 22.

The family-zero packet changes to subtype one and presentation 21013 while
retaining magical attack, magical defense, and magical hit rate. Resource
11000027 appears at the hero immediately. Table 204 then determines a fan of
resource-10000080 warning blades spread around the target direction; the
shipped table's final column controls the width calculation.

At the marker delay those warnings are replaced by resource-10000081 blades,
starting 180 world units from the hero. They travel straight for at most 90
updates and expire on scenery or their first target. Their packet carries the
directional 21021 and 21022 presentations. The final launch plays sample 94,
contacts play sample 20, and successful contacts train spell twelve.

### Lightning Storm

Lightning Storm still requires a pointed enemy, but only to choose the cast
direction. Action 35 uses CAF charts 11 and 12. Effect 10013 receives a fixed
hero origin and deliberately stores `-1` for both its source and target
identities; damage ownership remains in the copied player packet.

That family-zero subtype-zero packet uses magical attack and hit rate,
physical defense, presentation 20005, and the spell's Table 17 banks. Table
204 determines how many rays form each circle. Starting 350 world units from
the origin, four circles are attempted four updates apart at radii 350, 550,
750, and 950.

Every clear ray creates resources 10000030, 10000031, and 10000032. Only the
first layer applies the packet in its 100-unit area, using a randomized chart.
Each ray remembers its own scenery obstruction, so a blocked direction stops
on later circles without suppressing the others. Sample 21 plays once per
attempted circle, and successful contacts train spell thirteen.

### Medusa

Medusa returns to a single pointed projectile. Action 36 uses CAF charts 13
and 14, requires a living enemy, and passes effect 10014 the Table 17 travel
speed, height 200, target identity, aim direction, hero judgement rectangle,
and chart-marker delay.

Its family-zero packet uses subtype two, magical attack, magical defense,
magical hit rate, and presentation 21019. Effect 10014 has no separate source
visual. At the authored delay it starts resource 10000070 180 world units in
front of the hero. The projectile uses 80-unit bounds, moves straight, and
expires on scenery or its first target. Sample 22 plays at launch, sample 20
plays on contact, and a successful receiver contact trains spell fourteen.

### Sonic Blade

Sonic Blade is the first spell whose action is driven by the equipped weapon.
It still needs a pointed living enemy and pays the ordinary Table 16 MP cost,
but an empty hand or any weapon other than subtype zero, three, or one rejects
the cast without spending MP. Those three subtypes use CAF chart pairs 5/6,
15/16, and 19/20 respectively, with the normal attack-speed tiers rather than
Table 20's casting-speed factors.

Action 37 immediately shows effect 21025 from resource 11000100. Each newly
crossed first-chart `0x40` marker plays sample 154 and launches effect 10015,
whose resource-10000090 blade begins 200 world units in front of the hero. It
travels at Table 17's speed with `[-80,-80,79,79]` bounds, display height 155,
and a fixed seven-update lifetime, ending on scenery or its first target.
Contact plays sample 20 and trains spell fifteen. Action counter six also
plays the equipped weapon's normal attack sample.

Unlike the preceding spells, its family-zero packet is physical type zero.
Damage is Table 17 parameter zero percent of the hero's physical attack, with
a minimum of one; word five uses physical defense, word 34 is presentation
21024, and word 72 is one. Retail still adds the hero's magical hit rate to
Table 17 parameter one for the packet accuracy value.

### Mud Javelin

Mud Javelin returns to the ordinary pointed-spell path. Action 38 uses CAF
charts 13 and 14 at the Table 20 row-sixteen rate, requires a living enemy,
and creates effect 10016 as soon as the action begins. Its authored marker
still determines when that controller launches the visible attack.

The family-zero subtype-three packet adds Table 17 parameter zero to magical
attack and parameter one to magical hit rate. It carries magical defense,
Table 17 parameter five, and a randomly selected ordinary impact presentation
from 21000 through 21003. The selected enemy, Table 17 travel speed, hero
judgement rectangle, marker delay, and constructor field 22 all remain owned
by the effect request.

At the marker delay, resource 10000110 launches with 80-unit contact bounds
and sample 19. The controller follows that projectile until it disappears,
then places resource 10000111 at its last position. On the burst's fifth
update it applies the packet to every target in its 240-unit area, plays sample
22, and requests the nearby camera shake. Successful receiver contacts train
spell sixteen.

### Identify

Identify is a self-cast with action 39, CAF charts 11 and 12, and the casting
rate from Table 20 row seventeen. Starting the cast shows effect 21028 from
resource 11000230. When the first-chart `0x40` marker is crossed, the Inventory
panel opens on the right and the normal system cursor changes to its authored
Identify pattern.

While this mode is active, only an unidentified item in the backpack can be
chosen. Equipment, belt items, special items, an item currently held by the
pointer, and already identified items are ignored. A successful choice marks
the item as identified, reveals its full name and values, gives Identify one
practice point, and leaves Inventory open. Right-clicking or closing Inventory
cancels the mode. Casting Identify again while it is already waiting for an
item is consumed without another MP charge or casting action.

The identified bit belongs to the item instance and is saved with it. Retail
mirrors it in word 48 for weapons and armor and word 47 for accessories.
Before identification, the tooltip uses the item's base description as its
name and does not expose the hidden values.

Malse's `Identify Items` service is a separate scenario-script operation, not
an invocation of this spell. Once his post–Red Goblin merchant menu is
available, opcode 55 scans equipped gear, accessories, the backpack, and the
belt for any unidentified instance. When at least one exists, the authored
dialogue substitutes the flat 100-Gold price and selects `NO` by default. A
confirmed purchase checks and spends the player's total Gold, then opcode 4
identifies every eligible owned item in one pass. The script has distinct
branches for insufficient Gold and for a character whose items are already
identified.

### Merchant repairs

Malse's post–Red Goblin menu also offers the retail Repair Items service. It
quotes Arms, Head Armor, Body Armor, Shield, Leg Armor, All Equipped Items, and
Non-Equipped Items without opening another inventory surface. Arms and Shield
include both active and alternate weapon sets; Non-Equipped means repairable
weapons and armor in the backpack only.

Retail derives each price from the item's full generated value and Table 34:
`(missing durability * (item value / 10)) / maximum durability`. Integer
division is used throughout, and a damaged item whose result rounds to zero
still costs one Gold. A fully repaired item contributes zero. Successful
payment restores current durability to the definition's maximum and updates
the same raw item-instance word written to a retail save. The script handles
already-repaired and insufficient-Gold choices before it asks the item owners
to mutate anything.

### Medicine and companion food

Category-three items in the backpack and belt share the executable's one use
path. Player life and mana are tried first. Both flat restoration and the
maximum-pool percentage are scaled by the matching equipped base bonus, using
the live derived maximum HP or MP and retail's integer operation order. An item
is consumed and sample 16 plays as soon as either player pool changes.

Only when neither player pool changed does the command try the owned companion.
Meat, Quality Meat, High Quality Meat, and Excellent Quality Meat restore their
Table-backed flat companion-life values. The companion must be alive and below
maximum life; food used on a full or defeated companion remains in its owner
and produces no use sound. Condition and elemental medicines continue after
this same priority chain.

White Medicine resets the player's two saved element axes to `(0,0)`. Fire,
Water, Earth, Thunder, Holy, Dark, Gel, and Metal Medicine each move that point
4,000 units toward their fixed retail element anchor, snapping exactly to the
anchor when it is closer than one step. These are persistent element-alignment
changes rather than timed buffs. Using a medicine that cannot move or clear
the point leaves the item untouched. The Status marker, eight displayed
affinities, offensive and defensive combat calculations, and retail save all
consume the same axes.

## Character Stats

### Primary Stats
- Maximum HP
- Maximum MP
- Strength
- Magic Level
- Walking Speed

### Combat Stats
- Attack
- Defense
- Hit Rate
- Evasion Rate
- Magical Attack
- Magical Defense
- Magical Hit Rate
- Magical Evasion Rate
- Speed of Attack

### Companion Stats (mirrors player stats)
- Companion HP
- Companion Attack
- Companion Defense
- Companion Hit Rate
- Companion Evasion Rate
- Companion Magical Attack
- Companion Magical Defense
- Companion Magical Hit Rate
- Companion Magical Evasion Rate

### Recovery Stats
- Life Recovery (per second, %)
- Mental Recovery (per second, %)
- Mental Consumption (per second, %)

### Special Combat Stats
- Duration of Stiffness
- Probability of Stiffness
- Reflection Rate (damage reflection %)
- Generation of Reflection (chance to trigger %)
- Absorption Rate (damage absorption %)
- Incidents of Absorption (chance to trigger %)
- Speed of Chant (casting speed)

### Resource Stats
- Amount of Gold (+%)
- Effect of Mine (damage)
- Number of Mines (available)
- Effect of Stamina Medicine (+%)

## Player Death and Recovery

Without a revival item, lethal damage selects player action 5. Retail locks
ordinary input, plays animation chart 4 in direction 8, and holds its final
frame for 120 game updates. It then returns the hero to the current scenario
entry with the transition's revive flag set. The revive reset fills both HP
and MP to their current maximums and clears the death action.

This means death is not a normal state that can be saved from the settings
menu. A zero-life save produced by an older portable build is an invalid
state, not ordinary retail save behavior.
- Effect of Mental Medicine (+%)

## Attack Modes

Multi-way attacks available:
- 2WAY
- 3WAY
- 5WAY
- 7WAY

Special attack properties:
- High damage attack
- Avoidance of counter attacks

## Settings/Options

Found in the options menu (0x004103c0):

| Setting | Options |
|---------|---------|
| Screen Mode at Start | WINDOW / FULL |
| Semi-transparent Objects | ON / OFF |
| Semi-transparent Shadow | ON / OFF |
| Display Darkness | ON / OFF |
| Save Image at Game End | ON / OFF |
| Click Range | ON / OFF |
| Click Range | MINI / SMAL / NORM / LARG / MAX. |
| Click Priority | ENEM / OBJ. / ITEM / PEOP / COMP |
| EFF.VOLUME | Mute, then -3000 through 0 |
| BGM VOLUME | Mute, then -3000 through 0 |

The portable build intentionally keeps the original screen-mode row empty and
always opens in a window. The gap remains, so every later setting keeps its
retail screen coordinate. Clicking one of the five priority labels moves that
class to the right-hand end and shifts the lower-priority classes left.

### Target Types
- ENEM (Enemy)
- OBJ. (Object)
- ITEM (Item)
- PEOP (People)
- COMP (Companion)

## Land Mines

Land Mines are a separate player resource, not an inventory stack or spell.
A new character starts with five. Pressing `B`, or clicking the mine cell at
`496..511,424..439`, places one at the hero's current world position and
starts a ten-update placement lockout. The HUD click plays sample 58; the
keyboard path does not.

The placed mine uses static OPTION resource 1000 with a 300-by-300 judgement
box. It arms on update 40 and then plays positional sample 54 every 20 updates.
Contact with a living enemy or active scenario object triggers it; an
untriggered mine also expires into its explosion at update 300. Changing maps
or relocating through a scenario entry clears placed mines without restoring
the spent count.

The explosion uses OPTION resource 1001, sample 29, and a 1200-by-1200 area
which can hit every enemy inside it. Damage comes from Table 23 at the level
captured when the mine was placed, plus the equipped mine-effect bonus, with a
minimum of one. Resources 1002 through 1004 form the expanding debris rings;
1005 through 1008 and paired 1004 pieces make the four bouncing fragments.
The controller finishes at update 80 after the explosion.

Mine items are category four, definition one. Picking one up increments the
separate counter while it is below the current maximum. It never enters the
backpack. At maximum capacity the pickup is rejected through the normal world
drop response, so the mine bounces and plays its landing sound instead of
silently disappearing. The inventory panel shows the Mine icon whenever the
count is nonzero, followed by the current count and the live maximum. The base
maximum is ten; equipped instance word 84 raises it, while instance word 81
raises mine damage. The current count is saved after the magic block in the
retail `.Ssv` stream and survives portable save/load as well.

`Item.Ibn` gives the Mine definition the generic weight field value one, but
retail does not multiply that field by the mine counter. Its live weight
routine at `0x00445630` visits only the nine equipped item pointers, so mines
do not change the inventory Weight number or attack-speed encumbrance.

## Controls (from help text)

### Mouse Actions

| Action | Control |
|--------|---------|
| Attack enemy | L-click on Enemy |
| Attack while moving | R-click |
| Attack when not moving | R-click |
| Use Magic | R-click |
| Check Targets | TAB+L-click on Enemy / TAB+R-click on Enemy |
| Pick up items | TAB+L-click on Items |
| Select attack target | SHIFT+L-click |
| Interact with near object | L-click Near Object |

### R-click Actions Menu
- Attack While Moving
- Attack When Not Moving
- Use Magic
- Check Targets
- Companion's Attack
- Companion's Dash
- Let Companion Get Items

### Keyboard Actions
- Use Medicine in Belt Pocket
- Open Navigation Window
- Land Mines
- Increased-Power Mode On
- Run
- Action without Movement
- Companion Active/Inactive
- Open Help Window
- Chat (with Shift for Chat with All)

### Window Shortcuts
- Open Status Window
- Open Item Window
- Open Magic Window
- Open Special Item Window
- Open Mission List Window (`Q`; two pages of 24 table-backed missions)
- Open Navigation Window/Map (`N`; the world remains live in the right half,
  arrow keys scroll the Map, and Enter recenters it)
- Open Settings Menu

### Other Actions
- Switch Walk/Run
- Get Screen Shots (VK_SNAPSHOT/PrintScreen)
- Producers (credits)
- Save Image for Load Screen
- TAB+L-click on Items

### Special Keys
- Enter/Return - Confirm/chat mode (handled at 0x00402840)
- PrintScreen - Screenshot (sets flag at 0x0048D71C)

## Network/Multiplayer

The game supports multiplayer through RKC_NETWORK:

| Mode | Value | Description |
|------|-------|-------------|
| 0    | Single Player | Local game |
| 1    | Client | Connect to server |
| 2    | Server | Host game |

Network error messages:
- "Network Errors have occurred."
- "Network Communication Error!"
- "Network Connection Error!"

Player join/leave messages:
- "Player [%s] login to this world."
- "Player [%s] logout from this world."
