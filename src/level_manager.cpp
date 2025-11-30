#include "level_manager.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

LevelManager::LevelManager()
	: current_spawn_radius(INITIAL_SPAWN_RADIUS)
	, circle_count(0)
{
}

float LevelManager::get_spawn_radius() const
{
	return current_spawn_radius;
}

int LevelManager::get_circle_count() const
{
	return circle_count;
}

void LevelManager::start_new_circle()
{
	// Increment circle count
	circle_count++;
	
	// Expand the spawn radius by adding the increase amount
	// This creates a bigger circle around the previous circle
	current_spawn_radius += RADIUS_INCREASE_PER_CIRCLE;
}

void LevelManager::reset()
{
	current_spawn_radius = INITIAL_SPAWN_RADIUS;
	circle_count = 0;
}

float LevelManager::get_required_survival_time_seconds() const
{
	return REQUIRED_SURVIVAL_TIME_SECONDS;
}

int LevelManager::get_required_kill_count() const
{
	return REQUIRED_KILL_COUNT;
}

// ===== DIFFICULTY SCALING IMPLEMENTATIONS =====

int LevelManager::get_base_enemy_health(int level) const
{
	// Base health increases linearly with level
	// Level 1: 100, Level 2: 120, Level 3: 140, etc.
	return BASE_ENEMY_HEALTH + static_cast<int>((level - 1) * HEALTH_PER_LEVEL);
}

int LevelManager::get_base_enemy_damage(int level) const
{
	// Base damage increases linearly with level
	// Level 1: 10, Level 2: 12, Level 3: 14, etc.
	return BASE_ENEMY_DAMAGE + static_cast<int>((level - 1) * DAMAGE_PER_LEVEL);
}

float LevelManager::get_enemy_health_multiplier(int level, float time_in_level_seconds) const
{
	// Start with base health for this level
	float base_multiplier = 1.0f;
	
	// Apply time-based scaling if we've been in the level long enough
	if (time_in_level_seconds > TIME_SCALING_START_SECONDS) {
		float time_into_scaling = time_in_level_seconds - TIME_SCALING_START_SECONDS;
		float scaling_progress = std::min(1.0f, time_into_scaling / TIME_SCALING_RAMP_SECONDS);
		
		scaling_progress = std::pow(scaling_progress, TIME_SCALING_CURVE);
		
		float time_multiplier = 1.0f + (scaling_progress * (MAX_TIME_HEALTH_MULTIPLIER - 1.0f));
		
		base_multiplier = time_multiplier;
	}
	
	return base_multiplier;
}

float LevelManager::get_enemy_damage_multiplier(int level, float time_in_level_seconds) const
{
	// Start with level-based multiplier
	float level_multiplier = 1.0f + ((level - 1) * (DAMAGE_PER_LEVEL / BASE_ENEMY_DAMAGE));
	
	// Apply time-based scaling if we've been in the level long enough
	if (time_in_level_seconds > TIME_SCALING_START_SECONDS) {
		float time_into_scaling = time_in_level_seconds - TIME_SCALING_START_SECONDS;
		float scaling_progress = std::min(1.0f, time_into_scaling / TIME_SCALING_RAMP_SECONDS);
		
		scaling_progress = std::pow(scaling_progress, TIME_SCALING_CURVE);
		
		float time_multiplier = 1.0f + (scaling_progress * (MAX_TIME_DAMAGE_MULTIPLIER - 1.0f));
		
		// Combine level and time multipliers
		return level_multiplier * time_multiplier;
	}
	
	return level_multiplier;
}

float LevelManager::get_enemy_spawn_multiplier(int level, float time_in_level_seconds) const
{
	// Start with level-based multiplier
	float level_multiplier = 1.0f + ((level - 1) * SPAWN_MULTIPLIER_PER_LEVEL);
	
	// Apply time-based scaling if we've been in the level long enough
	if (time_in_level_seconds > TIME_SCALING_START_SECONDS) {
		float time_into_scaling = time_in_level_seconds - TIME_SCALING_START_SECONDS;
		float scaling_progress = std::min(1.0f, time_into_scaling / TIME_SCALING_RAMP_SECONDS);
		
		scaling_progress = std::pow(scaling_progress, TIME_SCALING_CURVE);
		
		float time_multiplier = 1.0f + (scaling_progress * (MAX_TIME_SPAWN_MULTIPLIER - 1.0f));
		
		// Combine level and time multipliers
		return level_multiplier * time_multiplier;
	}
	
	return level_multiplier;
}

