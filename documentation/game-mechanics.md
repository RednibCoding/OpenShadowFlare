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
| Moon | (none) | Unknown effect |
| Transport | (none) | Teleportation |
| Identify | (none) | Identify items |

### Spell Stats
Each spell has:
- **Lv** - Spell level
- **Exp** - Experience towards next level
- **MP** - Mana cost
- **Effect %** - Heal%, Def%, Shield%, Attack%, RefPer% (reflection percent)

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
| Click Range | MINI / SMAL / NORM / LARG / MAX. |

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
- Open Mission List Window
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
