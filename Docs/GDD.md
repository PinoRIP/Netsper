# Game Design Document
## Project Title (Working Title): Netsper

---

# 1. Elevator Pitch

A high-velocity first-person arena shooter that merges the precision of classic competitive FPS games with the fluid movement of anime-inspired action systems.

Players compete as elite cyber-athletes in futuristic arenas where speed, precision, and creativity determine victory.

Combat is built around the philosophy:

**“Stylish eSper Shooting Sports.”**

Rather than focusing purely on aim, the game emphasizes:

- movement mastery  
- spatial awareness  
- expressive combat  
- strategic ability usage  

Matches resemble a high-speed futuristic sport where players weave across walls, launch into aerial duels, and chain stylish melee combos while maintaining precise gunplay.

The result is a competitive experience that rewards mastery and creativity, turning every match into a spectacular ballet of speed and skill.

---

# 2. Game Overview

## Genre

Competitive Arena FPS  
Movement Shooter  
Team-Based Objective Shooter

---

## Perspective

**True First Person**

The player always experiences the game through a first-person camera.

Reasons:

- stronger sense of speed
- higher skill ceiling
- better immersion
- competitive readability

---

## Match Structure

Typical match length:

8 – 15 minutes

Match sizes depend on gamemode

---

# 3. Core Vision

The core experience centers around **high-skill mechanical gameplay combined with expressive movement**.

Players should feel like **professional cyber-athletes competing in a futuristic sport.**

---

## Player Fantasy

Players embody elite competitors inside a stylized digital arena.

They experience:

- gravity-defying movement
- stylish melee duels
- precise gunfights
- tactical team coordination
- momentum-driven combat

The fantasy is **speed, mastery, and spectacle.**

---

# 4. Design Pillars

## Pillar 1 — Movement Is Skill

Movement defines the skill ceiling of the entire game.

Mastery of movement allows players to:

- control engagement distance
- escape dangerous situations
- initiate combat advantageously
- maintain momentum across the map

Movement is based on **momentum and velocity** rather than animation locking.

This allows:

- advanced techniques
- creative traversal
- player expression

---

### Movement System

Movement consumes **SP (Stamina Points)**.

SP is a regenerating resource that limits mobility spam while still allowing high-skill mobility chains.

---

### Core Movement Mechanics

#### Running

Base movement state.

Balanced speed allowing accurate weapon usage.

---

#### Sprinting

Inspired by S4 League.

Consumes SP.

Used for:

- repositioning
- escaping fights
- gaining momentum for jumps

---

#### Jumping

Inspired by Apex Legends.

Allows:

- momentum carry
- chaining with other movement mechanics

---

#### Crouching

Reduces player height.

Uses:

- slide initiation
- recoil stabilization
- cover usage

---

#### Sliding

Inspired by Apex Legends.

Features:

- momentum preservation
- ramp-sensitive speed boosts
- SP usage for extended slides

---

#### Wallrunning

Inspired by Mirror's Edge.

Duration of the wallrun is determined by the initial velocity and angle of approach and the wall's slope.

Allows players to:

- traverse vertical surfaces
- reposition mid-fight

---

#### Walljumping

Hybrid between S4 League and Mirror’s Edge.

Players can:

- rebound off walls
- chain multiple wall jumps
- redirect momentum

Advanced players can create **movement routes through maps.**

---

#### Wall Climbing

Inspired by Apex Legends.

Allows short vertical climbs.

---

#### Mantling

Allows players to smoothly climb over ledges.

Maintains movement flow.

SP can be used to perform **faster ascents**. Normal mantle is a normal climb up vs SP-boosted mantle which is a pull up powerfull enough to be like a jump up.

---

#### Dodge

Inspired by S4 League.

Quick directional dash.

Includes:

- ground dodge
- air dodge

Uses SP.

Key defensive tool.

---

#### Landing Mechanics

Inspired by Mirror's Edge.

High-velocity landings cause **stagger**.

Players can perform a **roll** to:

- prevent stagger
- convert downward velocity into forward momentum

This rewards skilled timing.

---

# 5. Combat System

Combat combines **three core systems**:

1. Ranged weapons
2. Melee combat
3. eSper abilities

These systems are designed to complement movement rather than replace it.

---

## Combat Range Philosophy

Assuming equally skilled players:

| Range | Dominant System |
|------|----------------|
| Close | Melee |
| Mid | Movement + Mixed |
| Long | Ranged |

However, player skill and positioning can override these expectations.

---

# 6. Ranged Weapons

Gunplay is designed to feel **responsive, accurate, and competitive**.

Inspired by modern shooters but adapted to high mobility gameplay.

---

### Core Weapon Types

#### Assault Rifle

- mid-range control
- reliable damage
- versatile weapon

---

#### SMG

- high fire rate
- strong in close range
- weaker at distance

---

#### Shotgun

- high burst damage
- close-range dominance

---

#### Sniper Rifle

- long range
- high skill requirement
- potential one-shot headshots

---

#### Pistol

- reliable sidearm
- quick weapon swap

---

### Gunplay Philosophy

Weapons emphasize:

- strong hit feedback
- readable recoil
- clear projectile or hitscan behavior

Players should feel in full control of their shots even while moving quickly.

---

# 7. Melee Weapons

Melee combat is a defining feature of the game.

Inspired by:

- S4 League
- anime action games
- fighting games

Melee weapons allow combos and stylish finishes.

---

### Key Principles

Melee weapons:

- excel in close range
- allow combo chains
- benefit from movement momentum
- extend player mobility options

Attacks are **not root-motion locked**, allowing players to maintain control.

---

### Example Melee Weapons

#### Plasma Sword

High speed.

Good for aggressive players.

---

#### Katana

Balanced melee weapon.

Allows fluid combos.

---

#### Counter Sword

Defensive melee weapon.

Allows parry-style gameplay.

---

#### Scythe

Wide sweeping attacks.

Strong against grouped enemies.

---

# 8. eSper Ability System

Abilities represent energy powers used by cyber athletes.

Abilities enhance gameplay but **never replace mechanical skill.**

---

### Ability Examples

#### Grapple

Inspired by Apex.

Allows quick repositioning.

Shorter range than typical grapples.

---

#### Shield

Deployable defensive barrier.

Blocks projectiles.

---

#### Wall

Temporary cover creation.

Useful in objective play.

---

#### Flight

Short burst aerial movement.

Allows vertical combat opportunities.

---

#### Invisibility

Temporary stealth ability.

Used for flanking or escapes.

---

# 9. Loadout System

Players can equip:

- **0–3 weapons**
- **1 ability**

---

### Loadout Modes

Two gameplay structures exist.

---

### Arena Mode

Players collect weapons and abilities inside the map.

Inspired by classic arena shooters.

Focus on:

- map control
- resource denial

---

### Competitive Loadout Mode

Players enter the match with a pre-selected loadout.

Focus on:

- playstyle specialization
- team composition

---

# 10. Game Modes

## Touchdown

Inspired by sport-like arena gameplay.

**Format:** 5v5

Objective:

Teams must carry a ball into the opponent's goal.

### Rules

- The ball carrier **constantly loses SP**.
- The ball **can only be dropped if the carrier is killed**.
- Passing mechanics may be introduced in later iterations.

### Strategic Impact

Encourages:

- coordinated escorts
- aggressive interceptions
- movement mastery under pressure

---

## Chaser

An asymmetric free-for-all mode.

**One player becomes the Chaser** and receives significant buffs.

The rest of the lobby must attempt to survive and defeat them.

### Chaser Buffs

- increased health
- increased damage
- enhanced mobility

### Scoring

**Points for the Chaser**

- Eliminating the players in scorboard order → **4 points**
- Eliminating a player out of order → **2 points**

**Points for Players**

- Winning against the Chaser → **1 point for all non-Chaser players**
- Surviving until the end → **+1 additional point**

**Killing the Chaser**

Players receive points based on **damage contribution ranking**:

| Damage Rank | Points |
|-------------|--------|
| 1st | 6 |
| 2nd | 4 |
| 3rd | 3 |
| 4th | 2 |
| 5th | 1 |

Encourages coordinated pressure and aggressive play.

---

## 1v1 Arena

A **team-based duel ladder mode**.

Two teams face each other in **sequential 1v1 fights**.
			   

### Rules

- Only **two players fight at a time**.
- When one player dies, the **next teammate spawns in**.
- Players waiting for their turn **spectate their teammate**.

### Victory Condition

The team that eliminates all opponents wins.

### Design Purpose

Encourages:

- mechanical mastery
- clutch plays
- spectator excitement

---

## BetterRoyal
				 

A competitive extraction-based mode inspired by battle royale concepts.

Players spawn with **dog tags**.

### Dog Tag Mechanics

- Each player spawns with **1 dog tag**.
- Dog tags can be **consumed to scan for nearby tags**.
- 1 tag = 1 scan.

### Scanning System
				  

Scanning reveals:

- dropped dog tags
- players carrying dog tags

The more dog tags a player carries, the **greater the scanning radius**.

---

### Tag Economy

When a player is killed:

- all dog tags they carry are dropped

Dog tags serve multiple purposes:

- scanning
- reviving teammates
- extraction requirements

---

### Reviving

Players can revive teammates using **any dog tag**.

This creates strategic decisions between:

- reviving teammates
- saving tags for extraction

---

### Extraction

Teams must collect **10 dog tags** to qualify for extraction.

Extraction points become available once the requirement is met.

---

# 11. Map Design

Maps must support **high-speed traversal.**

Key features:

- vertical layers
- wallrun surfaces
- open combat arenas
- flanking routes

---

### Map Design Goals

Maps should encourage:

- movement routes
- advanced traversal techniques
- strategic positioning

---

### Map Size

Medium sized arenas optimized for:

- 3v3
- 4v4
- 5v5

---

# 12. Camera System

True first person.

The camera reacts to:

- movement speed
- landing impact
- directional momentum

Subtle camera effects emphasize speed without reducing clarity.

---

# 13. Character System (Priority 2)

Characters exist primarily for **visual identity**.

Characters do not restrict gameplay roles.

Players can:

- select different characters
- customize outfits
- unlock cosmetics

---

# 14. Progression System (Priority 2)

Progression focuses on **cosmetic rewards.**

---

## Player Level

Players earn:

- experience
- currency
- cosmetics

---

## Unlockables

Possible rewards:

- skins
- weapon effects
- emotes
- victory animations

---

# 15. Competitive Design Philosophy

Core principles:

**Skill > Gear**

Player skill should always be the primary determinant of victory.

---

**Movement > Positioning**

Players can actively change positioning through skillful movement.

---

**Counterplay Exists**

Every weapon and ability must have counters.

No dominant strategy.

---

# 16. Spectator and eSports Vision

The game is designed for competitive play.

Future systems may include:

- spectator camera
- replay tools
- tournament modes
- ranked ladder

Fast movement and stylish combat make the game highly watchable.

---

# 17. Future System Extensions

The design allows for expansion without compromising the core pillars.

Possible additions:

- new melee weapons
- new abilities
- seasonal maps
- limited-time modes

All additions must respect the core philosophy:

**Speed, Skill, Style.**

---

# 18. Development Priorities

## Priority 1

Core gameplay systems:

- movement
- gunplay
- melee combat
- networking
- map design

---

## Priority 2

Secondary systems:

- characters
- cosmetics
- progression

---

## Priority 3

Live service features:

- seasonal content
- esports tools
- events