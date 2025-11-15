#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class MinimapSystem
{
public:
	MinimapSystem();
	~MinimapSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif

	// Update minimap with player position
	void update_player_position(Entity player_entity, float spawn_radius, vec2 spawn_position, int circle_count);
	
	// Update bonfire position on minimap (pass empty vec2 to hide it)
	void update_bonfire_position(vec2 bonfire_world_pos, float spawn_radius, vec2 spawn_position);

	// Render the minimap UI
	void render();

	void play_intro_animation();
	void set_visible(bool visible);

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* minimap_document = nullptr;
#endif
	
	// Minimap configuration
	const float MINIMAP_RADIUS = 160.0f; // Half of 320px container
	float minimap_world_view_radius = 3200.0f; // How much of the world to show (in pixels) - scales with spawn radius
	
	// Circle radius constants (matching LevelManager)
	const float INITIAL_SPAWN_RADIUS = 1600.0f;
	const float RADIUS_INCREASE_PER_CIRCLE = 800.0f;
	
	// Helper method to update all circle elements
	void update_circles(int circle_count, float current_spawn_radius, vec2 spawn_position);
};

