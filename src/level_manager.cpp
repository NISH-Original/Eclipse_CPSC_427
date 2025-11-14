#include "level_manager.hpp"
#include <iostream>

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
	
	std::cerr << "New circle started! Circle count: " << circle_count 
	          << ", New spawn radius: " << current_spawn_radius << std::endl;
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

