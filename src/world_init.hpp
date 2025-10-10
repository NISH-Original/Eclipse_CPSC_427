#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"


// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// the bullet
Entity createBullet(RenderSystem* renderer, vec2 pos, vec2 velocity);

// Enemies
Entity createEnemy(RenderSystem* renderer, vec2 pos);
Entity createSlime(RenderSystem* renderer, vec2 pos);

