#include "minimap_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>
#include <cmath>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

MinimapSystem::MinimapSystem()
{
}

MinimapSystem::~MinimapSystem()
{
#ifdef HAVE_RMLUI
	if (minimap_document) {
		minimap_document->Close();
	}
#endif
}

#ifdef HAVE_RMLUI
bool MinimapSystem::init(Rml::Context* context)
#else
bool MinimapSystem::init(void* context)
#endif
{
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null for minimap" << std::endl;
		return false;
	}

	minimap_document = rml_context->LoadDocument("ui/minimap.rml");
	
	if (!minimap_document) {
		std::cerr << "ERROR: Failed to load minimap document" << std::endl;
		return false;
	}

	minimap_document->Show();
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - Minimap disabled" << std::endl;
	return false;
#endif
}

void MinimapSystem::update_player_position(Entity player_entity, float spawn_radius, vec2 spawn_position)
{
#ifdef HAVE_RMLUI
	if (!minimap_document || !registry.motions.has(player_entity)) {
		return;
	}
	
	Motion& player_motion = registry.motions.get(player_entity);
	vec2 player_pos = player_motion.position;
	
	// Minimap shows MINIMAP_WORLD_VIEW_RADIUS pixels of world space
	// The spawn position is at the center of the minimap
	float world_to_minimap_scale = MINIMAP_RADIUS / MINIMAP_WORLD_VIEW_RADIUS;
	
	// Calculate player position relative to spawn
	vec2 player_relative_to_spawn = player_pos - spawn_position;
	
	// Convert to minimap coordinates
	float player_minimap_x = player_relative_to_spawn.x * world_to_minimap_scale;
	float player_minimap_y = player_relative_to_spawn.y * world_to_minimap_scale;
	
	// Position player dot on minimap (relative to center)
	float player_dot_x = MINIMAP_RADIUS + player_minimap_x - 6; // -6 to center the 12px dot
	float player_dot_y = MINIMAP_RADIUS + player_minimap_y - 6;
	
	// Clamp player dot to stay within minimap bounds
	float max_offset = MINIMAP_RADIUS - 15; // Leave margin for the dot
	float distance_from_center = sqrt(player_minimap_x * player_minimap_x + player_minimap_y * player_minimap_y);
	
	if (distance_from_center > max_offset) {
		float clamp_scale = max_offset / distance_from_center;
		player_minimap_x *= clamp_scale;
		player_minimap_y *= clamp_scale;
		player_dot_x = MINIMAP_RADIUS + player_minimap_x - 6;
		player_dot_y = MINIMAP_RADIUS + player_minimap_y - 6;
	}
	
	// Update player dot position
	Rml::Element* player_dot = minimap_document->GetElementById("player_dot");
	if (player_dot) {
		char left_str[32];
		char top_str[32];
		snprintf(left_str, sizeof(left_str), "%.1fpx", player_dot_x);
		snprintf(top_str, sizeof(top_str), "%.1fpx", player_dot_y);
		player_dot->SetProperty("left", left_str);
		player_dot->SetProperty("top", top_str);
	}
	
	// Spawn radius circle stays centered and fixed size
	// (already positioned correctly in CSS at center)
#else
	(void)player_entity;
	(void)spawn_radius;
	(void)spawn_position;
#endif
}

void MinimapSystem::render()
{
}

