# Eclipse

A game by Team Saturday (Canvas Team #14)

- Durukan Butun (23857824)
- Nanjou Olsen (59393686)
- Alan Zheng (18887208)
- Chowdhury Zayn Ud-Din Shams (46176756)
- Han Kim (45508413)
- Will Beaulieu (24994386)
- Nishant Molleti (21147343)

## Overview

Eclipse is a top-down, horde survival shooter, revolving around the story of a character who has crash-landed on an alien planet perpetually eclipsed in darkness.
Their goal is to escape the shadow of the eclipse being cast on the planet by another celestial body by defeating the hostile aliens that get in their way.

Our twist on the horde survival genre is our realistic lighting system, which immerses the player in the world of the game.

## User Inputs

Main Controls
- `W`/`A`/`S`/`D`: Move player up/left/down/right
- `MOUSE`: Aim flashlight and gun
- `LEFT CLICK`: Shoot gun
- `I`: Open inventory
- `ESCAPE`: Close game window

Debug Controls
- `G`: Re-generate world elements
- `O`: Toggle ambient lighting
- `R`: Restart game
- `CTRL + R`: Refresh UI

## Proposal

Eclipse by Team Saturday: [proposal.pdf](doc/proposal.pdf)

## Milestone 1

### Current Game State

<!-- TODO: explain M1 elements -->

![Game Screenshot](doc/screenshot_m1.png)

### Required Elements

#### `[1] Rendering: Textured geometry`
- The enemy "Slime" uses a 6 frame spritesheet for its animation
- The randomly-placed trees in the world are rendered using a single sprite

#### `[2] Rendering: Basic 2D transformations`
- The placeholder enemy faces and follows the player
- The placeholder enemy spins and shrinks during its death animation

#### `[3] Rendering: Key-frame/state interpolation`
- The movement and rotation of the placeholder enemy is calculated with respect to `elasped_ms`, the enemy's position, and the player's position
- The slime enemy's current animation frame is calculated respect to `elasped_ms`
  
#### `[4] Gameplay: Keyboard/mouse control`
- The player uses the WASD keys to move around the world, and the I key to open the inventory
- The player uses the mouse to aim the flashlight, and left-click to shoot a bullet

#### `[5] Gameplay: Random/coded action`
- All enemies are coded to face and move towards the player
 - All trees are placed at random positions in the world
  
#### `[6] Gameplay: Well-defined game-space boundaries`
- The player can not move beyond the window border
- Obstacles are not generated outside of the window border

#### `[7] Gameplay: Simple collision detection & resolution (e.g. between square sprites)`
- Enemies die when hit by bullets

#### `[8] Stability: Stable frame rate and minimal game lag`
- Our game runs smoothly on sufficiently-powerful hardware
- Our game may lag on lower-end devices due to our advanced lighting systems

#### `[9] README: All README sections and sub-sections above are completed as described`
- All sections and sub-sections are completed
  
#### `[10] Software Engineering: Test plan that covers game play and excepted outcomes`
- Our test plan is [here](doc/test_plan.pdf)

#### `[11] Reporting: Using GitHub Issues, record any bugs`
- Our open issues are [here](https://github.students.cs.ubc.ca/CPSC427-2025W-T1/team14/issues)
- Our closed issues are [here](https://github.students.cs.ubc.ca/CPSC427-2025W-T1/team14/issues?q=is%3Aissue+is%3Aclosed)
  
### Creative Elements

#### `[1] Simple Rendering Effects (Basic)`
**Techniques:** Fragment shader color manipulation<br>
Making the fragments in the player view-cone brighter, along with fading out the brightness based on distance and angle.

#### `[7] 2D Dynamic Shadows (Advanced)`
**Techniques:** Basic shadow mapping, ray-object intersections<br>
Added dynamic shadows that move with entities. Uses raymarching to check occlusions. An entity can be marked as an occluder by adding the occluder component.<br>
Press 'o' to view the occlusion mask.

#### `[20] External Integration (Basic)`
**Techniques**: CMake knowledge<br>
We are using `RmlUi` to create our inventory UI, so we have integrated it (as well as `FreeType`) into our project.

### Other Notes

Source for the slime spritesheet: https://pixelmikazuki.itch.io/free-slime-enemy
