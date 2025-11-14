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
	void update_player_position(Entity player_entity, float spawn_radius, vec2 spawn_position);
	
	// Update bonfire position on minimap (pass empty vec2 to hide it)
	void update_bonfire_position(vec2 bonfire_world_pos, vec2 spawn_position);

	// Render the minimap UI
	void render();

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* minimap_document = nullptr;
#endif
	
	// Minimap configuration
	const float MINIMAP_RADIUS = 160.0f; // Half of 320px container
	const float MINIMAP_WORLD_VIEW_RADIUS = 3200.0f; // How much of the world to show (in pixels) - increased to show more area
};

