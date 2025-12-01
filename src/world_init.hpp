#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "noise_gen.hpp"
#include "render_system.hpp"
#include "level_manager.hpp"

// the player
Entity createPlayer(RenderSystem* renderer, vec2 pos);

// the player's feet
Entity createFeet(RenderSystem* renderer, vec2 pos, Entity parent_player);

// the player's dash sprite
Entity createDash(RenderSystem* renderer, vec2 pos, Entity parent_player);

// "tree" obstacle
Entity createTree(RenderSystem* renderer, vec2 pos, float scale);

// create a bonfire
Entity createBonfire(RenderSystem* renderer, vec2 pos);

// create an arrow that points toward the bonfire
Entity createArrow(RenderSystem* renderer);

// create a flashlight cone
Entity createFlashlight(RenderSystem* renderer, vec2 pos);

// create a background that can be affected by lighting
Entity createBackground(RenderSystem* renderer);

// the bullet
Entity createBullet(RenderSystem* renderer, vec2 pos, vec2 velocity, int damage = 20);
Entity createExplosionEffect(RenderSystem* renderer, vec2 pos, float radius);

void createBloodParticles(vec2 pos, vec2 bullet_vel, int count);
void createBossBloodParticles(vec2 pos, int count);
void createBeamParticlesCone(vec2 pos, vec2 dir_vel, int count, vec4 col);
void createDashParticles(vec2 pos, vec2 dash_dir);

Entity create_drop_trail(const Motion& src_motion, const Sprite& src_sprite);
Entity createXylarite(RenderSystem* renderer, vec2 pos);
Entity createFirstAid(RenderSystem* renderer, vec2 pos);

// Enemies
Entity createEnemy(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds);
Entity createXylariteCrab(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds);
Entity createSlime(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds);
Entity createEvilPlant(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds);

// create an enemy light
Entity createEnemyLight(RenderSystem* renderer, vec2 pos);

// create an isoline obstacle entity with circle colliders
std::vector<Entity> createIsolineObstacle(RenderSystem* renderer, vec2 pos, CHUNK_CELL_STATE iso_state);


std::vector<Entity> createIsolineCollisionCircles(vec2 pos, CHUNK_CELL_STATE iso_state);


void removeIsolineCollisionCircles(std::vector<Entity>& collision_entities);

// generate a new world chunk
Chunk& generateChunk(RenderSystem* renderer, vec2 chunk_pos, PerlinNoiseGenerator& map_noise, PerlinNoiseGenerator& decorator_noise,std::default_random_engine& rng, bool is_spawn_chunk = false, bool is_boss_chunk = false);
