#pragma once

#include <vector>
#include <functional>

#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"
#include "common.hpp"

class AudioSystem;

class AISystem
{
public:
	void init(RenderSystem* renderer, AudioSystem* audio);
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
	void dropStep(float step_seconds);
	void trailStep(float step_seconds);

	std::function<void()> on_enemy_killed;

	RenderSystem* renderer;
	AudioSystem* audio_system;
};