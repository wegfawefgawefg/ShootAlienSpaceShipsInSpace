#Shoot Alien Space Ships In Space Design

This document is the target for the C++ reboot from this point forward.

Progression and loot direction lives in:
- [progression.md](./progression.md)

The older Python repo is still the reference for the basic feel and assets:
- title screen
- ship movement
- bullets
- debris / enemy-style targets
- starfield
- audio

From here, the C++ version intentionally grows past the Python prototype.

## Core Direction

- Keep the code simple and direct.
- Do not bring back Lua, mods, rooms, crates, inventory, or other template-repo systems.
- Use plain per-frame game logic.
- Use simple collision checks for ships, enemies, and bullets.
- Prefer explicit structs, arrays, and straightforward update passes.

## Window And Presentation

- Window size: `1280x720`
- Default behavior: centered and floating-friendly for tiling WM setups like i3
- Keep the retro asset scale and simple sprite rendering
- Use the starfield warp effect as a transition visual between levels
- Render the gameplay world to a half-resolution intermediate texture: `640x360`
- Render the UI directly at full window resolution
- Keep the world pixelated and the overlay crisp
- The world view should integer-scale to fill the window, not sit in a small floating center box.
- UI should stay compact and sit as full-resolution overlays in corners instead of stealing large gutters.
- `Tab` should open a paused inventory / inspection view with categorized lists and detailed stat readouts.
- The inventory view should eventually show per-weapon attached upgrades, not only a flat upgrade list.
- Stat readouts should prefer vertical, spelled-out labels over compressed abbreviations.
- Mouse hover and keyboard focus should both expose detailed descriptions and derived numbers.
- The shop should be a real between-level phase, not just an implicit reward screen.
- The shop should support both keyboard and mouse:
  - hover to inspect
  - click / Enter to buy
  - reroll for gold
  - inspect current loadout without leaving the shop
  - equip / stash / sell during the shop phase

## Controls

- Move: arrow keys and `WASD`
- Shoot: `Space`
- `Esc` / `Q`: back to title or quit from title

The player keeps control during level transitions, but shooting is disabled during the transition window.

## Camera

- Keep the combat camera fixed in a strong readable shmup framing.
- Do not slide the gameplay camera around with the player during normal play.
- Replace camera motion with camera shake for impact.
- The starfield can still parallax / drift in response to player movement and warp.
- UI must stay in screen space and never obstruct the main play area.
- The combat framing should be a little closer than the last pass.

## Player And Party

- The player can have wingman ships later.
- Wingmen should fly in a loose downward V / escort pattern near the player.
- Each wingman has its own weapon list just like the player.
- Wingmen are not a separate special-case firing path; they use the same weapon model as the player.

## Weapons

The player does not have a single hardcoded gun. The player has a list of weapon objects.

- Weapons stack.
- Picking up another weapon adds another firing behavior instead of replacing the old one.
- A weapon can be simple or complex.
- A weapon can have ammo, reload rules, cooldown rules, spread rules, or explosion behavior.

### Fixture Types

Each weapon also has a fixture type that determines how it is mounted / fired:

- `center`
  - shoots from ship center

- `evenly_spread`
  - multiple weapons line up tightly across the ship and fire from those positions

- `splayed`
  - fires from center but with angular spread

### Weapon Examples

- basic single bullet
- missile
- delayed explosion shot
- limited-ammo weapon
- target-tracking shot
- split-on-explosion shot
- beam weapon using start / continue / end particle slices

Not every weapon needs to be implemented immediately, but the weapon model should allow them.

## Current Scene Flow

1. Title screen
2. Start battle on key press
3. Enter level transition
4. Show level text
5. Fly enemies into formation
6. Start live enemy behavior
7. When all enemies are dead, transition to next level

## Level Transition

Each level begins with a short transition phase:

- The player can move immediately.
- The player cannot shoot during the transition.
- A `Level N` text appears with a strong wiggle / drift.
- Enemies fly in toward their target formation positions.
- The starfield ramps into a warp-style effect to imply travel to a new location.
- `start_warping` should play when the transition begins.
- `stop_warping` should play when the transition ends and normal combat begins.
- Once the formation settles and the level text exits, normal combat begins.

Outside transitions, the starfield should return to the slower Python-style feel:

- many stars
- calmer base drift
- stronger edge acceleration only when warp ramps up

## Levels And Waves

- Cut scope to `4` levels for now.
- Each level contains multiple waves.
- Difficulty should rise across waves inside a level and again from one level to the next.
- Difficulty can increase through:
  - ship count
  - speed
  - size
  - hp
  - behavior complexity
- The last wave of each level should be a boss wave.

Wave rules:

- Normal waves can have a timer.
- If the player kills the current wave early, launch the next wave immediately.
- If the timer expires first, launch the next wave anyway even if earlier ships are still alive.
- Boss waves should not have a timer.

UI should show:

- current level
- current wave
- remaining wave timer when relevant

Between levels, the long-term structure should shift toward:

- combat
- boss
- reward summary
- shop
- next level transition

Current implemented slice:

- combat -> boss -> shop -> next level
- gold from damage / kills / leftover timer
- active weapon slots and a limited stash
- active weapon selling and stash refill
- inventory inspection while in combat or shop

## Debug Capture Plan

For further UI and gameplay tuning, add a CLI-driven debug bootstrap path instead of relying on
manual play to reach a useful state.

Desired tooling:

- launch directly into seeded battle / inventory / shop snapshots from the CLI
- use deterministic seeded state so comparisons are stable
- dump a PNG of the current frame to disk for inspection
- support a few canned snapshots:
  - early run
  - overloaded late run
  - shop state
  - boss/combat state

This is mainly for closing the tuning loop faster when adjusting:

- UI layout
- shop card readability
- inventory readability
- gameplay presentation
- camera / effects polish

## Next Tuning Pass

- add more charge/spool weapons or beam visual polish
- tune rarity/shop weighting now that the pool is broader
- tune the new grammars so they feel distinct and not over/underpowered

Hardcoded C++ data is fine for now. JSON is optional later if it becomes useful.

## Enemy Model

Enemy ship type and enemy behavior are separate concepts.

That means:
- any ship visual / type id can use any behavior
- behavior can change at runtime
- better enemy design can come from combining type, behavior, and facing mode instead of baking them together

This is important so that later enemies can switch behavior based on:
- nearby enemy density
- player position
- elapsed time
- damage state
- level script conditions

Future ship properties can also include split counts instead of hardcoded split-on-death behavior.

Example direction:
- `split_count = 2` means the ship can split into two children
- each child decrements its split stage
- `2 -> 1 -> 0` is valid
- not every ship should split
- size, visual scale, and post-split behavior should all be per-ship properties

That should remain an optional ship capability, not the default death rule for all enemies.

Enemies also need:
- hit points
- hit shake
- optional larger scale / size
- optional split_count and split stage
- optional `is_boss`
- optional boss behavior state

Enemy HP must allow enemies to take more than one hit.
Hit shake can just be a decaying float that adds random draw offset while active.

Enemies, the player, bullets, and other floating objects can also have a simple height model:

- `base_height`
- `height`
- `target_height`

Height can drive:

- shadow offset
- subtle bobbing
- impact sag when hit
- small motion accents during turning / movement

It should always ease back toward a target height instead of snapping.

## Enemy Behaviors

Initial behavior set:

1. Straight flight
   - Flies straight after the fly-in completes
   - Wraps around screen edges

2. Slow chase
   - Gradually moves toward the player

3. Top-screen shooter
   - Hangs near the top of the screen
   - Moves left and right periodically
   - Shoots downward or toward the player on a timer

4. Rammer
   - Lines up horizontally with the player
   - Backs up slightly
   - Accelerates hard into a ramming pass

5. Circler
   - Orbits around a center or flies in circular motion

6. Wander targeter
   - Picks a random target position on screen
   - Rotates toward it
   - Flies there
   - Repeats

## Boss Direction

Bosses are still enemies, but with extra properties and extra behavior orchestration.

- Beating a boss can unlock a simpler non-boss version later.
- Boss guns are still just guns / attack patterns, but some are reserved for bosses.
- Bosses should rotate between behaviors over time or by random selection.
- The available boss behavior set should change by HP thresholds:
  - above `50%`
  - between `50%` and `25%`
  - below `25%`

Example boss attack patterns:

- machine-gun spray
- rotating spoke bursts
- charging beam
- mine laying
- ramming phase

For debug UI, show at least:

- current boss behavior name
- current boss behavior group / phase

## Enemy Facing Modes

Enemies also have a separate facing rule:

1. Face player
2. Face movement direction
3. Face down

These are separate from behavior so the same movement logic can support different reads and personalities.

## World Rules

- Enemies can wrap around left/right/top/bottom
- Player does not wrap
- Player remains in the active play space
- Bullets and enemies use simple collision checks
- No heavy physics library
- Ships should never pop into view at their behavior target position.
- New wave enemies should enter from off-screen and fly into formation before behaving normally.

## Combat Feel

- Add hitstop / stop frames on hits and damage taken
- Use a generic stop-frame counter that can be increased by larger events
- Most hits should be small
- Bigger enemies / stronger hits can add more stop frames
- Shake on the player, enemies, and camera can be more aggressive than the current mild values.
- Run the simulation at `144 Hz` with fixed steps.
- Use substeps for better collision reliability so fast bullets do not step through enemies.

## Debug And UI

- Draw collider boxes or circles for ships and projectiles in debug mode
- Show current level in UI
- Show current weapon list in a side panel
- Weapon panel should at least show:
  - weapon type
  - particle / projectile type
  - fixture type
- Show player / party details in the UI
- UI must fit comfortably in the side panels at full resolution
- Collider shapes should be smaller than the full `8x8` sprite footprint when appropriate
- Show wave timer and wave index in the UI
- Show boss behavior name and group in the UI when a boss is active

## Code Shape

- Keep `src/` flat.
- Keep modules narrow and explicit:
  - `app`
  - `assets`
  - `battle_types`
  - `level_data`
  - `player_weapons`
  - `damage`
  - `enemy_behavior`
  - `level_flow`
  - `battle_render`
- Do not reintroduce the old Lua / menu / template stack.
- Keep files small enough to stay readable and easy to reason about.

## Tuning Backlog

- Title screen placement and scale still need tuning to match the Python reference better.
- The new fixed combat framing and shake values will need tuning.
- Starfield speed / warp ramp / parallax will need polish.
- Enemy formation arrival timing and level intro timing need tuning.
- Enemy collision / bullet readability need local playtesting.
- UI panel layout needs cleanup so it reads clearly without stealing too much space.
- Debug boxes should stay easy to toggle if they start cluttering combat.
- UI should stay out of the gameplay field

## Implementation Order

1. Fix shell issues: windowing, controls, starfield parity
2. Add a proper level state machine
3. Add transition text and warp transition timing
4. Add enemy struct and bullet struct separation for player/enemy fire
5. Add formation targets
6. Add the first three behaviors
7. Add the remaining behaviors
8. Add level definitions for ten stages
9. Add stacked weapon objects and fixture types
10. Add enemy HP, hit shake, and stop frames
11. Add side-panel UI and debug collider rendering
12. Add simple follow camera and zoom
13. Add wingman / party support
14. Tune movement, shooting, pacing, and screen readability

## Non-Goals

- No attempt to preserve the accidental complexity of the current template repo
- No scripting layer for this phase
- No physics middleware for this phase
