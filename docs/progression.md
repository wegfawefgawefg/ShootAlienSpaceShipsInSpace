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

## Future Hooks

- item choices instead of pure vacuum collection
- weapon rarity
- curse items
- boss-exclusive attack modules
- wingmen with their own weapon inventories
- inventory caps / replacement decisions
- synergy bonuses by tag counts

The important thing now is getting the first pickup loop working in a clean way.
