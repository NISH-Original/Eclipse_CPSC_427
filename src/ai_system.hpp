#pragma once

#include <vector>
#include <functional>

#include "tiny_ecs_registry.hpp"
#include "common.hpp"

class AISystem
{
public:
	void step(float elapsed_ms);
	
	// Set callback for when an enemy is killed
	void set_kill_callback(std::function<void()> callback) { on_enemy_killed = callback; }

	AISystem()
	{
	}
private:
	void enemyStep(float elapsed_ms);
	void spriteStep(float elapsed_ms);
	
	std::function<void()> on_enemy_killed;
};