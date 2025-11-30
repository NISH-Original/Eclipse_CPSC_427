#pragma once

#include "common.hpp"

// Manages level progression through expanding circles
// Each circle represents a new exploration area unlocked by interacting with bonfires
class LevelManager
{
public:
	LevelManager();
	
	// Get the current spawn radius for the current circle
	float get_spawn_radius() const;
	
	// Get the number of circles/bonfires the player has passed
	int get_circle_count() const;
	
	// Start a new circle - expands the spawn radius around the previous circle
	// This is called when the player interacts with a bonfire to open inventory
	void start_new_circle();
	
	// Reset to initial state (for game restart)
	void reset();

	void set_circle_count(int count) { circle_count = count; }
	void set_spawn_radius(float radius) { current_spawn_radius = radius; }

	// Get objective requirements for the current level
	// These are the same for all levels (for now)
	float get_required_survival_time_seconds() const;
	int get_required_kill_count() const;

	// ===== DIFFICULTY SCALING METHODS =====
	
	// Get health multiplier for enemies based on level and time in level
	float get_enemy_health_multiplier(int level, float time_in_level_seconds) const;
	
	// Get damage multiplier for enemies based on level and time in level
	float get_enemy_damage_multiplier(int level, float time_in_level_seconds) const;
	
	// Get spawn count multiplier for enemies based on level and time in level
	float get_enemy_spawn_multiplier(int level, float time_in_level_seconds) const;
	
	// Get base enemy health for a given level (before time scaling)
	int get_base_enemy_health(int level) const;
	
	// Get base enemy damage for a given level (before time scaling)
	int get_base_enemy_damage(int level) const;

private:
	// Current spawn radius for the active circle
	float current_spawn_radius;
	
	// Number of circles/bonfires the player has interacted with
	int circle_count;
	
	// Initial spawn radius (starting circle)
	const float INITIAL_SPAWN_RADIUS = 1600.0f;
	
	// Radius increase per circle (makes each new circle bigger)
	const float RADIUS_INCREASE_PER_CIRCLE = 0.0f;
	
	// Objective requirements (same for all levels)
	const float REQUIRED_SURVIVAL_TIME_SECONDS = 60.0f;
	const int REQUIRED_KILL_COUNT = 10;
	const int BASE_KILL_COUNT = 3;
	const float KILL_COUNT_MULTIPLIER = 1.4f;
	
	// ===== DIFFICULTY SCALING CONSTANTS =====
	
	// Base stats per level (between-level scaling)
	const int BASE_ENEMY_HEALTH = 100;
	const int BASE_ENEMY_DAMAGE = 10;
	const float HEALTH_PER_LEVEL = 20.0f;  // +20 health per level
	const float DAMAGE_PER_LEVEL = 2.0f;   // +2 damage per level
	const float SPAWN_MULTIPLIER_PER_LEVEL = 0.15f;  // +15% spawns per level
	
	// Time-based scaling (within-level scaling)
	const float TIME_SCALING_START_SECONDS = 60.0f;  // Start scaling after 1 minute
	const float TIME_SCALING_RAMP_SECONDS = 120.0f;  // Ramp up over next 2 minutes (total 3 min)
	const float MAX_TIME_HEALTH_MULTIPLIER = 3.0f;   // Max 3x health at 3+ minutes
	const float MAX_TIME_DAMAGE_MULTIPLIER = 3.0f;   // Max 3x damage at 3+ minutes
	const float MAX_TIME_SPAWN_MULTIPLIER = 4.0f;    // Max 4.0x spawns at 3+ minutes
	const float TIME_SCALING_CURVE = 2.0f;  // Exponential curve (2 = quadratic)
};

