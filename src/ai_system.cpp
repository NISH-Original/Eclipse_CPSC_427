// internal
#include "ai_system.hpp"
#include <iostream>

void AISystem::step(float elapsed_ms)
{
	float step_seconds = elapsed_ms / 1000.f;
	enemyStep(step_seconds);
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
			motion.angle += 3 * M_PI * step_seconds;
			motion.velocity = {0.0f, 0.0f};
			motion.scale -= glm::vec2(30.0f) * step_seconds;

			if (motion.scale.x < 0.f || motion.scale.y < 0.f) {
				registry.remove_all_components_of(entity);
			}
		} else {
			glm::vec2 diff = player_motion.position - motion.position;
			motion.angle = atan2(diff.y, diff.x);
			motion.velocity = glm::normalize(diff) * 50.f;

		}
	}
}
