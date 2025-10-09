// internal
#include "ai_system.hpp"
#include <iostream>

void AISystem::step(float elapsed_ms)
{
	enemyStep(elapsed_ms);
}

void AISystem::enemyStep(float elapsed_ms)
{
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& enemy_registry = registry.enemies;
	
	for(uint i = 0; i< enemy_registry.size(); i++) {
		Entity entity = enemy_registry.entities[i];
		Motion& motion = registry.motions.get(entity);
		glm::vec2 diff = player_motion.position - motion.position; 
		motion.velocity = glm::normalize(diff) * 50.f;
	}
}
