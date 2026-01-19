# Eclipse

A game by Team Saturday
- Durukan Butun
- Nanjou Olsen
- Alan Zheng
- Chowdhury Zayn Ud-Din Shams
- Han Kim
- Will Beaulieu
- Nishant Molleti

## Overview

Eclipse is a top-down, horde survival shooter, revolving around the story of a character who has crash-landed on an alien planet perpetually eclipsed in darkness.
Their goal is to escape the shadow of the eclipse being cast on the planet by another celestial body by defeating the hostile aliens that get in their way.

Our twist on the horde survival genre is our realistic lighting system, which immerses the player in the world of the game.

The game engine is built from the ground up with **C++** and **OpenGL**.

## User Inputs

Main Controls
- `W`/`A`/`S`/`D`: Move player up/left/down/right
- `MOUSE`: Aim flashlight and gun
- `LEFT CLICK`: Shoot gun
- `R`: Reload gun
- `E`: Interact with campfire
- `LSHIFT`: Dash
- `ESCAPE`: Return to main menu

Debug Controls
- `C`: Show circular and mesh-based bounding boxes of the player.
- `G`: Re-generate world
- `B`: Trigger boss battle
- `=`: Restart game
- `[`: Open inventory
- `]`: Add 1000 Xylarite
- `CTRL + R`: Refresh UI
- `F5`: Save game
- `F9`: Load game from save


## Core Systems

### Engine Architecture (ECS-Based)
- Entity-Component-System (ECS) architecture for clean separation of data and behavior  
- Systems operate on component queries for cache-friendly iteration  
- Modular design enables rapid feature iteration (weapons, enemies, upgrades, UI)  
- Centralized event and messaging system for decoupled gameplay logic  
- Deterministic update loop for stable real-time simulation

### Rendering Backend (C++ / OpenGL)
- Custom OpenGL renderer for 2D sprite rendering and lighting passes  
- Multi-pass rendering pipeline for lighting, shadows, and post-effects  
- GLSL shader system for dynamic lighting, SDF generation, and particles  
- Batched sprite rendering for reduced draw calls  
- GPU instancing for high-performance particle effects

### Dynamic Lighting & Shadows (GPU-Accelerated)
- Real-time soft shadow system using a Signed Distance Field (SDF) and GPU ray marching in GLSL  
- SDF recomputed each frame for all obstacles  
- Fragment shader performs sphere tracing toward light sources  
- Multi-ring sampling produces soft penumbra  
- Distance-based attenuation simulates light height for 2.5D depth  
- Supports multiple dynamic lights at ~60 FPS

### Procedural Infinite World Generation
- World generated in chunks using thresholded Perlin noise to define obstacle regions  
- Obstacles placed without overlapping the player or each other  
- Off-screen chunks serialized into a compact format and reloaded when needed  
- Dynamic bonfire spawning provides progression markers and light sources

### Collision & Physics
- Hybrid collision model:  
  - Mesh-based collisions for player damage detection  
  - Circular bounding boxes for physics pushing and world interactions  
- Optional debug visualization for collision volumes

### Camera System
- Smooth camera follow behavior during exploration  
- Cinematic transitions to and from campfires  
- Dynamic centering during gameplay and menu states

## Gameplay Architecture

### Combat & Weapons
- Multiple weapons including a rifle, each with upgradeable stats  
- Weapon upgrades and player progression via a unified upgrade system  
- Knockback physics, hit-flash feedback, and temporary enemy health bars improve combat readability

### Enemies & AI
- Swarm behavior using an adapted BOIDS model for flocking and collision avoidance  
- Pathfinding enemies navigate around obstacles  
- State-machine-driven ranged enemies (e.g., Evil Plant)  
- Boss enemy with:  
  - Multi-pattern attack logic  
  - Custom skinned tentacle animation (8 tentacles × 16 bones)  
  - Bone transforms loaded from DragonBones rig data

### Progression & Objectives
- Campfire system for level progression  
- Enemy scaling and content pacing for extended play sessions  
- Currency drops (Xylarite) and healing items (First Aid Kits)

## UI, Audio & Feedback

### HUD & UX
- Context-sensitive interactive tutorial  
- Inventory UI with item icons and tooltips  
- Objective tracker, stats display, and FPS counter  
- Directional arrow replaces the minimap for clarity  
- Audio toggle built into the HUD

### Audio System
- Event-driven sound effects (shooting, reloading, hits, damage)  
- Ambient background music  
- Real-time audio feedback integrated into gameplay actions

## Rendering & Visual Effects

### Particle System (Instanced Rendering)
- GPU instancing for efficient particle batching  
- Parameterized velocity and lifetime  
- Used for blood splatter, boss beam attacks, and environmental effects

### Animation
- Sprite-sheet animations for player, enemies, and bonfires  
- Custom skeletal animation for the boss using hierarchical bone transforms

## Persistence & Tooling

### Save / Load System
- Game state auto-saved when entering the menu  
- Serialized data includes:  
  - Map state  
  - Player stats  
  - Weapons  
  - Inventory  
- Enemies excluded to preserve pacing  
- Save files reloadable at startup or via debug keys

### Debug & Developer Tools
- World regeneration  
- Boss spawn trigger  
- Collision visualization  
- Inventory and currency injection  
- Manual save/load hotkeys  
- UI refresh and forced restarts

## Stability & Performance

### Runtime Stability
- Memory profiling using CRT debugging and AddressSanitizer  
- Major leak sources fixed; remaining leaks are minor  
- Robust input handling, including minimize / focus changes

## References

Source for the slime sprite sheet: https://pixelmikazuki.itch.io/free-slime-enemy  
Source for the player sprite: https://opengameart.org/content/animated-top-down-survivor-player

Sources referenced for Perlin noise implementation:
- Ken Perlin's 2002 paper about improvements to the original Perlin noise algorithm: https://doi.org/10.1145/566654.566636
- An archived slide deck by Ken Perlin about the original Perlin noise algorithm: https://web.archive.org/web/20071008162042/http://www.noisemachine.com/talk1/index.html
  - Sourced from https://en.wikipedia.org/wiki/Perlin_noise
- Perlin noise algorithm explanation by Raouf, including a reference implementation: https://rtouti.github.io/graphics/perlin-noise-algorithm
- Perlin noise algorithm explanation by Adrian, including a reference implementation: https://adrianb.io/2014/08/09/perlinnoise.html
