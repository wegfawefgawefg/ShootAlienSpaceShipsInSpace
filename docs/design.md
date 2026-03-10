# Shoot Alien Space Ships In Space Design

This document is the target for the C++ reboot from this point forward.

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
- Render the gameplay world to a low-resolution intermediate texture
- Render the UI directly at full window resolution
- Keep the world pixelated and the overlay crisp

## Controls

- Move: arrow keys and `WASD`
- Shoot: `Space`
- `Esc` / `Q`: back to title or quit from title

The player keeps control during level transitions, but shooting is disabled during the transition window.

## Camera

- The game needs a real camera instead of a fixed locked screen.
- The camera should pan with the player.
- The camera should be able to zoom out when the fight gets busy.
- Camera motion should be smooth, not snapped.
- UI must stay in screen space and never obstruct the main play area.
- The current camera should bias toward a slightly more zoomed-out view than the first port pass.

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

## Levels

- Start with `10` hardcoded levels.
- Each level defines:
  - enemy spawn entries
  - a target formation layout
  - enemy type ids
  - behavior assignments
  - facing assignments

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

## Combat Feel

- Add hitstop / stop frames on hits and damage taken
- Use a generic stop-frame counter that can be increased by larger events
- Most hits should be small
- Bigger enemies / stronger hits can add more stop frames

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
- Camera follow and zoom need gameplay feel tuning.
- Starfield speed / warp ramp still need polish.
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
