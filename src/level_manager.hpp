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
	
	// Kill count scaling constants
	const int BASE_KILL_COUNT = 10;
	const float KILL_COUNT_MULTIPLIER = 1.0f;
};

