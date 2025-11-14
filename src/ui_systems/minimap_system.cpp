#include "minimap_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

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

	// Initialize player dot styling
	Rml::Element* player_dot = minimap_document->GetElementById("player_dot");
	if (player_dot) {
		player_dot->SetProperty("background-color", "rgb(50, 200, 50)");
		player_dot->SetProperty("width", "12px");
		player_dot->SetProperty("height", "12px");
		player_dot->SetProperty("border-radius", "6px");
		player_dot->SetProperty("border", "2px #FFFFFF");
	}
	
	// Initialize bonfire dot as hidden
	Rml::Element* bonfire_dot = minimap_document->GetElementById("bonfire_dot");
	if (bonfire_dot) {
		bonfire_dot->SetProperty("display", "none");
		bonfire_dot->SetProperty("background-color", "rgb(255, 0, 0)");
		bonfire_dot->SetProperty("width", "10px");
		bonfire_dot->SetProperty("height", "10px");
		bonfire_dot->SetProperty("border-radius", "5px");
		bonfire_dot->SetProperty("border", "1px #FFFFFF");
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
	
	// Update player dot position and ensure styling is applied
	Rml::Element* player_dot = minimap_document->GetElementById("player_dot");
	if (player_dot) {
		char left_str[32];
		char top_str[32];
		snprintf(left_str, sizeof(left_str), "%.1fpx", player_dot_x);
		snprintf(top_str, sizeof(top_str), "%.1fpx", player_dot_y);
		player_dot->SetProperty("left", left_str);
		player_dot->SetProperty("top", top_str);
		// Explicitly set background-color to ensure it renders
		player_dot->SetProperty("background-color", "rgb(50, 200, 50)");
		player_dot->SetProperty("width", "12px");
		player_dot->SetProperty("height", "12px");
		player_dot->SetProperty("border-radius", "6px");
	}
	
	// Update spawn radius circle size based on actual spawn radius
	// Scale the world spawn radius to minimap coordinates
	// The spawn radius circle should scale proportionally with SPAWN_RADIUS
	// Scale it relative to MINIMAP_WORLD_VIEW_RADIUS to ensure it fits within the minimap
	float spawn_radius_minimap = (spawn_radius / MINIMAP_WORLD_VIEW_RADIUS) * MINIMAP_RADIUS;
	// Clamp to ensure it doesn't exceed the minimap bounds
	spawn_radius_minimap = std::min(spawn_radius_minimap, MINIMAP_RADIUS * 0.95f);
	float spawn_radius_diameter = spawn_radius_minimap * 2.0f;
	float spawn_radius_half = spawn_radius_minimap;
	
	Rml::Element* spawn_radius_circle = minimap_document->GetElementById("spawn_radius");
	if (spawn_radius_circle) {
		char width_str[32];
		char height_str[32];
		char margin_str[32];
		char border_radius_str[32];
		snprintf(width_str, sizeof(width_str), "%.1fpx", spawn_radius_diameter);
		snprintf(height_str, sizeof(height_str), "%.1fpx", spawn_radius_diameter);
		snprintf(margin_str, sizeof(margin_str), "%.1fpx", -spawn_radius_half);
		snprintf(border_radius_str, sizeof(border_radius_str), "%.1fpx", spawn_radius_half);
		spawn_radius_circle->SetProperty("width", width_str);
		spawn_radius_circle->SetProperty("height", height_str);
		spawn_radius_circle->SetProperty("margin-left", margin_str);
		spawn_radius_circle->SetProperty("margin-top", margin_str);
		spawn_radius_circle->SetProperty("border-radius", border_radius_str);
	}
#else
	(void)player_entity;
	(void)spawn_radius;
	(void)spawn_position;
#endif
}

void MinimapSystem::update_bonfire_position(vec2 bonfire_world_pos, vec2 spawn_position)
{
#ifdef HAVE_RMLUI
	if (!minimap_document) {
		return;
	}
	
	Rml::Element* bonfire_dot = minimap_document->GetElementById("bonfire_dot");
	if (!bonfire_dot) {
		std::cerr << "ERROR: bonfire_dot element not found in minimap!" << std::endl;
		return;
	}
	
	// If bonfire_world_pos is (0,0) or invalid, hide the dot
	if (bonfire_world_pos.x == 0.0f && bonfire_world_pos.y == 0.0f) {
		bonfire_dot->SetProperty("display", "none");
		return;
	}
	
	// Show the dot
	bonfire_dot->SetProperty("display", "block");
	
	// Minimap shows MINIMAP_WORLD_VIEW_RADIUS pixels of world space
	// The spawn position is at the center of the minimap
	float world_to_minimap_scale = MINIMAP_RADIUS / MINIMAP_WORLD_VIEW_RADIUS;
	
	// Calculate bonfire position relative to spawn
	vec2 bonfire_relative_to_spawn = bonfire_world_pos - spawn_position;
	
	// Convert to minimap coordinates
	float bonfire_minimap_x = bonfire_relative_to_spawn.x * world_to_minimap_scale;
	float bonfire_minimap_y = bonfire_relative_to_spawn.y * world_to_minimap_scale;
	
	// Position bonfire dot on minimap (relative to center)
	float bonfire_dot_x = MINIMAP_RADIUS + bonfire_minimap_x - 5; // -5 to center the 10px dot
	float bonfire_dot_y = MINIMAP_RADIUS + bonfire_minimap_y - 5;
	
	// Clamp bonfire dot to stay within minimap bounds
	float max_offset = MINIMAP_RADIUS - 10; // Leave margin for the dot
	float distance_from_center = sqrt(bonfire_minimap_x * bonfire_minimap_x + bonfire_minimap_y * bonfire_minimap_y);
	
	if (distance_from_center > max_offset) {
		float clamp_scale = max_offset / distance_from_center;
		bonfire_minimap_x *= clamp_scale;
		bonfire_minimap_y *= clamp_scale;
		bonfire_dot_x = MINIMAP_RADIUS + bonfire_minimap_x - 5;
		bonfire_dot_y = MINIMAP_RADIUS + bonfire_minimap_y - 5;
	}
	
	// Update bonfire dot position and ensure styling is applied
	char left_str[32];
	char top_str[32];
	snprintf(left_str, sizeof(left_str), "%.1fpx", bonfire_dot_x);
	snprintf(top_str, sizeof(top_str), "%.1fpx", bonfire_dot_y);
	bonfire_dot->SetProperty("left", left_str);
	bonfire_dot->SetProperty("top", top_str);
	// Explicitly set background-color to ensure it renders
	bonfire_dot->SetProperty("background-color", "rgb(255, 0, 0)");
	bonfire_dot->SetProperty("width", "10px");
	bonfire_dot->SetProperty("height", "10px");
	bonfire_dot->SetProperty("border-radius", "5px");
#else
	(void)bonfire_world_pos;
	(void)spawn_position;
#endif
}

void MinimapSystem::render()
{
}

