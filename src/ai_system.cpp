// internal
#include "ai_system.hpp"
#include <iostream>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923  // π/2
#endif

void AISystem::step(float elapsed_ms)
{
	float step_seconds = elapsed_ms / 1000.f;
	enemyStep(step_seconds);
	spriteStep(step_seconds); // Should be at the very end to overwrite motion
	stationaryEnemyStep(step_seconds);
}

void AISystem::enemyStep(float step_seconds)
{
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& enemy_registry = registry.enemies;
	
	for(uint i = 0; i< enemy_registry.size(); i++) {
		Entity entity = enemy_registry.entities[i];
		Enemy& enemy = registry.enemies.get(entity);
		Motion& motion = registry.motions.get(entity);
		
		if (enemy.is_dead) {
			if(enemy.death_animation == NULL) {
				motion.angle += 3 * M_PI * step_seconds;
				motion.velocity = {0.0f, 0.0f};
				motion.scale -= glm::vec2(30.0f) * step_seconds;

				if (motion.scale.x < 0.f || motion.scale.y < 0.f) {
					registry.remove_all_components_of(entity);
					// Trigger kill callback if set
					if (on_enemy_killed) {
						on_enemy_killed();
					}
				}
			} else {
				enemy.death_animation(entity, step_seconds);
			}

		} else {
			//glm::vec2 diff = player_motion.position - motion.position;
			//motion.angle = atan2(diff.y, diff.x);
			//motion.velocity = glm::normalize(diff) * 50.f;

		}
	}
}

// Functiton for stationaryEnemies (evil plants)
// TODO: Refactor enemy logic into proper ECS components and systems
void AISystem::stationaryEnemyStep(float step_seconds)
{
	const float detection_radius = 200.f;
	const int spriteFaceRight = 3;
	const int spriteFaceLeft = 2;
	const int spriteFaceDown = 0;
	const int spriteFaceUp = 1;

	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& stationary_enemy_registry = registry.stationaryEnemies;
	
	for(uint i = 0; i< stationary_enemy_registry.size(); i++) {
		Entity entity = stationary_enemy_registry.entities[i];
		Enemy& enemy = registry.enemies.get(entity);
		
		if (enemy.is_dead) continue;
		
		Motion& motion = registry.motions.get(entity);
		motion.velocity = {0.0f, 0.0f};


		Sprite& sprite = registry.sprites.get(entity);
		sprite.should_flip = false;

		vec2 diff = player_motion.position - motion.position;
		float dist = sqrt(diff.x * diff.x + diff.y * diff.y);

		if (dist < detection_radius) {
			if (fabs(diff.x) > fabs(diff.y)) {
				sprite.curr_row = (diff.x > 0) ? spriteFaceRight : spriteFaceLeft;
			} else {
				sprite.curr_row = (diff.y > 0) ? spriteFaceDown : spriteFaceUp;
			}
		}
	}
}

// Functiton for stationaryEnemies (evil plants)
// TODO: Refactor enemy logic into proper ECS components and systems
void AISystem::stationaryEnemyStep(float step_seconds)
{
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& stationary_enemy_registry = registry.stationaryEnemies;
	
	for(uint i = 0; i< stationary_enemy_registry.size(); i++) {
		Entity entity = stationary_enemy_registry.entities[i];
		Enemy& enemy = registry.enemies.get(entity);
		
		if (enemy.is_dead) continue;
		
		Motion& motion = registry.motions.get(entity);
		motion.velocity = {0.0f, 0.0f};
	}
}

void AISystem::spriteStep(float step_seconds)
{
	auto& sprite_registry = registry.sprites;
	
	for(uint i = 0; i< sprite_registry.size(); i++) {
		Entity entity = sprite_registry.entities[i];
		Sprite& sprite = registry.sprites.get(entity);

		sprite.step_seconds_acc += step_seconds * sprite.animation_speed;
		sprite.curr_frame = (int)std::floor(sprite.step_seconds_acc) % sprite.total_frame;

		// Disable rotation for entity with sprites (but not for players)
		if (registry.motions.has(entity) && !registry.players.has(entity) && !registry.feet.has(entity)) {
			Motion& motion = registry.motions.get(entity);
			if (motion.angle > M_PI_2 || motion.angle < -M_PI_2) {
				sprite.should_flip = true;
			} else {
				sprite.should_flip = false;
			}
			motion.angle = 0;
		}
	}
}
