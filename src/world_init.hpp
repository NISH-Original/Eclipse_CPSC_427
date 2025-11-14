#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "noise_gen.hpp"
#include "render_system.hpp"

// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// the player's feet
Entity createFeet(RenderSystem* renderer, vec2 pos, Entity parent_player);

// "tree" obstacle
Entity createTree(RenderSystem* renderer, vec2 pos, unsigned int scale);

// create a bonfire
Entity createBonfire(RenderSystem* renderer, vec2 pos);

// create a flashlight cone
Entity createFlashlight(RenderSystem* renderer, vec2 pos);

// create a background that can be affected by lighting
Entity createBackground(RenderSystem* renderer);

// the bullet
Entity createBullet(RenderSystem* renderer, vec2 pos, vec2 velocity);

// Enemies
Entity createEnemy(RenderSystem* renderer, vec2 pos);
Entity createSlime(RenderSystem* renderer, vec2 pos);
Entity createEvilPlant(RenderSystem* renderer, vec2 pos);

// create an enemy light
Entity createEnemyLight(RenderSystem* renderer, vec2 pos);

// generate a new world chunk
Chunk& generate_chunk(RenderSystem* renderer, vec2 chunk_pos, PerlinNoiseGenerator noise_func, std::default_random_engine rng);
