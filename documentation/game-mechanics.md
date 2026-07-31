# ShadowFlare Game Mechanics

## Character Classes

The game has 3 base playable character classes:

| Class   | Gender | Description |
|---------|--------|-------------|
| Hunter  | Both   | Ranged combat specialist (different attack revision) |
| Warrior | Both   | Melee combat specialist |
| Wizard  | Male   | Magic specialist |
| Witch   | Female | Magic specialist (female Wizard) |

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
| Energy Shield | Shield % | Damage reduction |
| Magic Shield | Def % | Defense boost |
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
