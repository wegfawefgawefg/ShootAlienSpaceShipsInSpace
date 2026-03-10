# Shoot Alien Space Ships In Space Progression

This document defines the run-progression direction for the C++ version.

The goal is not just "damage up" and "fire rate up." The goal is:

- lots of build identity
- weird rule-changing items
- tradeoffs and downsides
- weapon growth that changes how the ship plays
- support for future wingmen / partner ships without special-case systems

## Core Model

The run is built from four layers:

1. Weapons
2. Weapon mods
3. Ship passives
4. Wingmen / support units

All four should be stackable.

The player should end up with a run that feels like a pile of interacting parts, not a linear gun level-up tree.

## Current State

The current playable slice is intentionally narrow.

What exists now:

- drops
- pickup actors
- a small starter pickup pool
- weapon grants
- some random weapon upgrades
- auto vs semi weapon state
- simple ship augments
- inventory view
- gold drops and banked money
- a between-level shop phase
- active weapon slots plus a stash
- mouse hover / click selection in shop and inventory
- per-weapon attached upgrade inspection
- first distinct weapon families:
  - missiles with splash
  - rail shots with piercing
  - arc shots with strong lateral sway
  - parked proximity mines

Why it still feels samey:

- most of the currently implemented weapons are still bullet-family weapons
- most current upgrades still resolve into center / spread / splay bullet variations
- the full weapon and modifier roster is not implemented yet

This was on purpose for the first pass. The goal was to prove:

- drop flow
- collection flow
- item application
- inventory inspection
- auto/semi trigger logic

The next phase is where the roster gets much wider.

## Design Rules

- Prefer composable effects over hardcoded one-off logic.
- Prefer effects with a shape change or constraint, not only pure upside.
- Downsides make items more interesting:
  - more spread
  - slower bullets
  - worse damage per shot
  - longer reload
  - tighter pickup requirements
  - more recoil / more drift
  - reduced boss damage
  - less control but bigger payoff
- Boss rewards should be stronger and weirder than normal drops.
- Normal enemies can drip-feed power.
- Wave clear and boss clear should feel like progress spikes.

## Loot Flow

### Drop Sources

- normal enemy kill
- elite enemy kill
- boss kill
- wave clear reward
- later: shop / choice room / secret wave

### Drop Behavior

- pickups can spawn from kills
- pickups spawned outside the reachable player space should steer into a safe lane first
- once inside the grab space, they drift downward
- if the player is near them, they magnet toward the player
- boss kills should guarantee loot
- normal kills should be a chance-based weighted roll
- the system should eventually support pity rules so dry streaks do not feel bad

### Loot Tiers

- `common`
- `uncommon`
- `rare`
- `boss`
- `cursed`

## System Shape

### WeaponDef / WeaponInstance

Weapon data should define:

- type
- fixture
- projectile tile
- projectile count
- cooldown
- auto / semi-auto
- damage
- projectile speed
- projectile radius
- projectile life
- spread

An instance can then be modified by pickups over the course of the run.

Weapon instances should eventually also track:

- a stable instance id
- a display name
- which upgrades are attached to that specific weapon
- current live stats after modifiers
- whether it is sellable
- whether it is a boss / exotic weapon

This matters because an item like `Auto Retrofit` should not just show up globally under "weapon upgrades" with no context. The player needs to be able to see:

- which weapon received the mod
- what changed on that weapon
- what the current derived numbers are

### Weapon Slots And Stash

Probable direction:

- active weapon slots are limited
- starting limit may be `5`
- extra owned weapons can live in a stash
- stash size can also be limited
- if both active slots and stash are full, some pickups should be skipped instead of becoming free sell fodder

This would create more meaningful shop decisions:

- buy now
- stash for later
- replace an active weapon
- sell / scrap an old weapon

Not all upgrades should require manual application, but many should eventually support:

- auto-apply now
- choose target weapon
- stash for later use

### Fixture Types

- `center`
  - fires from ship center
- `evenly_spread`
  - spawns projectiles across the ship width
- `splayed`
  - fires from center with angular spread

### PickupEffect Families

- grant a new weapon
- upgrade a random existing weapon
- upgrade all weapons in a category
- change trigger mode
- add wingmen
- buff the player ship
- change pickup behavior / economy
- add rule-bending effects
- grant currency
- discount / reroll / shop effects

### Rule-Bender / Joker-Style Effects

These are the high-value items. They should feel like they bend a rule of the run:

- first shot each second deals triple damage
- every 7th shot splits
- picking up loot emits a burst
- if no enemy is near, fire rate doubles
- if surrounded, spread widens and damage goes up
- beam kills lay mines
- missiles inherit bullet upgrades
- wingmen copy the lowest-cooldown weapon
- semi-auto weapons gain crit, auto weapons gain chip damage

These should often come with a cost.

## Initial Runtime Plan

First implementation slice:

- loot drops
- pickup actors in the world
- a weighted loot table
- a small starter pickup set
- weapon grants
- random weapon upgrades
- auto-fire toggle items
- visible pickup list in UI

Later slices:

- wingmen
- beam weapons
- mines
- targeted missiles
- boss-exclusive rewards
- choice rewards instead of purely random pickup grabs
- sell / reroll / pack systems
- hover cards with richer stat deltas and target previews
- stash management beyond simple equip / unequip

## Inspection And UX

The run will get too complex very quickly if the player cannot inspect it.

### Inventory View

The paused inventory view should eventually show:

- weapons
- under each weapon: attached upgrades
- ship augments
- wingmen
- economy modifiers

Weapon sections should not just show tiny compressed stat strings. They should show vertical, readable labels like:

- Damage
- Cooldown
- Projectile Count
- Projectile Speed
- Projectile Life
- Projectile Radius
- Trigger Mode
- Fixture

This should be legible first and compact second.

### Hover / Focus Details

Mouse hover and keyboard focus should both expose:

- full item description
- exact stat changes
- current attachment target
- rarity
- sell value
- shop price

Shop phase should also expose:

- which offers are weapon grants versus augments
- whether a granted weapon will land in an active slot or stash
- current active/stash occupancy
- reroll cost

This is especially important in the shop.

### Attachment Visibility

If a modifier affects a single weapon, the player must be able to see that directly next to the weapon.

Examples:

- `Auto Retrofit` should appear under the specific weapon it changed
- `Hair Trigger` should show which gun got the cooldown reduction
- `Heavy Slugs` should show which gun took the damage / cadence tradeoff

Global lists are still useful, but the source of truth should be the weapon entry itself.

## Starter Pickup Set

This is the first playable subset to implement now.

### Weapon Grants

- `Twin Blaster`
  - extra center gun
- `Needle Cannon`
  - automatic fast low-damage gun
- `Sidecar Spread`
  - evenly spread side mount
- `Missile Pod`
  - slower heavy shots
- `Prism Fan`
  - wide splayed burst
- `Heavy Lance`
  - high damage, slower cadence

These are only the first slice. They are not the intended final roster.

### Weapon Upgrades

- `Hair Trigger`
  - faster cooldown, lower damage
- `Heavy Slugs`
  - more damage, slower cooldown
- `Auto Retrofit`
  - makes a random weapon automatic, lowers its damage a bit
- `Scatter Caps`
  - more projectiles, less damage per projectile
- `Overcrank`
  - much faster fire, shorter projectile life
- `Mag Rails`
  - faster projectile speed, slightly slower cadence

### Ship / Utility

- `Hull Patch`
  - +1 life
- `Magnet Core`
  - larger pickup magnet
- `Glass Reactor`
  - stronger weapon damage, slightly larger player hurtbox
- `Stability Fins`
  - less camera shake, slightly lower projectile speed

## Economy And Shop Direction

The next big system after the first pickup slice should be gold and a Balatro-style between-level shop.

### Gold

Enemies should drop money in addition to or instead of item drops.

Use at least three visual sizes:

- small gold
- medium gold
- large gold

Drop rules:

- normal enemies drop small gold on death
- tougher enemies can shed gold on hit and more on death
- bosses should drop large gold bursts
- leftover wave timer should convert into bonus gold

Gold should:

- pop outward slightly on spawn
- steer inward if it spawns out of reachable space
- drift / magnet like pickups

### Shop Timing

The cleanest first version is:

- combat level
- boss
- reward summary
- shop
- next level transition

That gives the player a natural breath point.

### Shop Structure

Balatro is a good structure reference here:

- fixed number of slots
- item packs
- reroll
- scaling prices
- rarities
- ability to sell

The shop does not need to copy Balatro exactly, but it should copy its strengths:

- clear readable item cards
- visible economy
- meaningful buy / skip / sell decisions
- short downtime between combat phases

### Shop Contents

The shop can sell:

- weapons
- weapon upgrades
- ship augments
- wingmen
- packs
- rerolls
- removals / sellouts

### Packs

Packs are a strong fit for this game.

Examples:

- weapon pack
- missile pack
- beam pack
- cursed pack
- wingman pack
- economy pack
- boss-tech pack

Packs should offer a small reveal / choice moment, not just direct auto-grants.

### Sell / Scrap

The player should eventually be able to:

- sell a weapon
- sell an upgrade
- scrap a weak item for gold
- maybe merge or convert duplicate items

That keeps the run from becoming unreadable junk.

## Giant Content List

The first code slice does not need to implement all of this immediately. This is the target content pool.

### 30 Weapons

1. Peashooter
2. Pulse Rifle
3. Burst Carbine
4. Marksman Shot
5. Twin Blaster
6. Tri-Blaster
7. Wide Spreader
8. Needle Cannon
9. Plasma Bolt
10. Heavy Slug
11. Rail Shot
12. Chain Gun
13. Auto Cannon
14. Ricochet Blaster
15. Piercing Lance
16. Flame Stream
17. Ion Beam
18. Prism Beam
19. Charge Beam
20. Missile Pod
21. Swarm Missiles
22. Dumbfire Rocket
23. Proximity Mine
24. Cluster Mine
25. Bomb Lobber
26. Arc Caster
27. Spoke Emitter
28. Side Laser
29. Rear Turret
30. Orbit Drone Emitter

## Priority Follow-Up Weapon Slice

These are the best next weapons to implement because they most expand the feel quickly:

1. Burst Carbine
2. Chain Gun
3. Rail Shot
4. Ion Beam
5. Prism Beam
6. Charge Beam
7. Missile Pod
8. Swarm Missiles
9. Dumbfire Rocket
10. Proximity Mine
11. Cluster Mine
12. Arc Caster

That set gets the game away from "mostly center / spread bullets" much faster.

### 30 Weapon Mods

1. Hair Trigger
2. Heavy Slugs
3. Auto Retrofit
4. Scatter Caps
5. Overcrank
6. Mag Rails
7. Full Bore
8. Tight Choke
9. Wide Bore
10. Heat Sink
11. Double Feed
12. Triple Feed
13. Piercing Rounds
14. Bouncing Rounds
15. Explosive Tips
16. Tracking Package
17. Delayed Fuse
18. Crit Lattice
19. Armor Shred
20. Shield Crack
21. Ammo Refund
22. Echo Shot
23. Ghost Rounds
24. Charge Capacitor
25. Beam Extender
26. Missile Splitter
27. Mine Cluster
28. Hollow Point
29. Backfire Chamber
30. Unstable Core

## Priority Follow-Up Upgrade Slice

These are the best next upgrades to implement because they create more identity quickly:

1. Piercing Rounds
2. Bouncing Rounds
3. Explosive Tips
4. Tracking Package
5. Delayed Fuse
6. Echo Shot
7. Beam Extender
8. Missile Splitter
9. Mine Cluster
10. Hollow Point

### 25 Ship Passives

1. Hull Patch
2. Magnet Core
3. Glass Reactor
4. Stability Fins
5. Shield Layer
6. Regen Weave
7. Pickup Burst
8. Shockwave Plating
9. Crit Aura
10. Damage Aura
11. Fire Rate Aura
12. Projectile Size Up
13. Hitstop Booster
14. Invuln Streak
15. Boss Hunter
16. Elite Lure
17. Scrap Vacuum
18. Wave Clock
19. Front Shield
20. Orbit Shield
21. Collision Dampers
22. Revenge Burst
23. Heat Converter
24. Adaptive Core
25. Cursed Surge

### 15 Wingmen / Support Items

1. Left Drone
2. Right Drone
3. Rear Drone
4. V-Wing Pair
5. Beam Satellite
6. Missile Buddy
7. Mine Layer Drone
8. Scavenger Drone
9. Repair Drone
10. Shield Drone
11. Ram Drone
12. Orbit Gun Pair
13. Mirror Wingman
14. Sniper Wingman
15. Boss-Killer Drone

## Downsides And Constraints

To keep builds interesting, many items should include one of these:

- lower raw damage
- lower projectile life
- higher cooldown
- worse spread
- larger player hurtbox
- weaker boss damage
- higher recoil / drift
- pickup magnet penalty
- lower crit rate in exchange for stronger base damage
- semi-auto only or auto only constraints

Examples:

- `Hair Trigger`
  - +fire rate
  - -damage
- `Heavy Slugs`
  - +damage
  - -fire rate
- `Auto Retrofit`
  - makes a weapon automatic
  - reduces that weapon's damage
- `Glass Reactor`
  - boosts damage
  - enlarges player hurtbox slightly
- `Stability Fins`
  - lower camera shake
  - slightly weaker projectile speed

The more item cards feel like "interesting trade" instead of "number goes up," the better this system will be.

## Future Hooks

- item choices instead of pure vacuum collection
- weapon rarity
- curse items
- boss-exclusive attack modules
- wingmen with their own weapon inventories
- inventory caps / replacement decisions
- synergy bonuses by tag counts
- gold burst drops
- shop packs
- reroll economy
- sell / scrap economy
- hover tooltips everywhere

## Next Implementation Order

1. Better inventory readability
   - vertical stat presentation
   - per-weapon attached upgrades

2. More weapon families
   - beams
   - missiles
   - mines
   - burst / rail / chain variants

3. Gold
   - small / medium / large drops
   - pickup and banked money

4. Between-level shop
   - buy
   - reroll
   - sell
   - pack opening

5. Mouse hover / richer detail text

6. Wingmen

The important thing now is getting the first pickup loop working in a clean way.
