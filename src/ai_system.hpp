#pragma once

#include <vector>
#include <functional>

#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"
#include "common.hpp"

class AISystem
{
public:
	void init(RenderSystem* renderer);
	void step(float elapsed_ms);
	
	// Set callback for when an enemy is killed
	void set_kill_callback(std::function<void()> callback) { on_enemy_killed = callback; }

	AISystem()
	{
	}
private:
	void enemyStep(float step_seconds);
	void stationaryEnemyStep(float step_seconds);
	void spriteStep(float step_seconds);
	
	std::function<void()> on_enemy_killed;

	RenderSystem* renderer;
};