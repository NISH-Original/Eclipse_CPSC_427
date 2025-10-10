#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "render_system.hpp"


// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// create a flashlight cone
Entity createFlashlight(RenderSystem* renderer, vec2 pos);

// create a background that can be affected by lighting
Entity createBackground(RenderSystem* renderer);

// the bullet
Entity createBullet(RenderSystem* renderer, vec2 pos, vec2 velocity);

Entity createEnemy(RenderSystem* renderer, vec2 pos);

// create an enemy light
Entity createEnemyLight(RenderSystem* renderer, vec2 pos);

