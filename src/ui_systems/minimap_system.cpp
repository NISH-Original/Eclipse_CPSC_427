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
	set_visible(false);
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - Minimap disabled" << std::endl;
	return false;
#endif
}

void MinimapSystem::update_player_position(Entity player_entity, float spawn_radius, vec2 spawn_position, int circle_count, const std::vector<vec2>& circle_bonfire_positions)
{
#ifdef HAVE_RMLUI
	if (!minimap_document || !registry.motions.has(player_entity)) {
		return;
	}
	
	// Scale minimap world view radius with spawn radius
	// Use a scaling factor to maintain proportional view (2x spawn radius by default)
	// This ensures the minimap shows more area as the spawn radius increases
	minimap_world_view_radius = spawn_radius * 1.25f;
	
	// Get the most recent/biggest circle's center position (this is what the minimap should center on)
	// Circle 0 uses initial spawn, Circle N (N > 0) uses bonfire from circle N-1
	vec2 minimap_center;
	if (circle_count == 0) {
		// Circle 0: center at initial spawn position
		if (circle_bonfire_positions.size() > 0) {
			minimap_center = circle_bonfire_positions[0];
		} else {
			minimap_center = spawn_position; // Fallback
		}
	} else if (circle_count < circle_bonfire_positions.size()) {
		// Circle N: center at bonfire from previous circle (stored at index circle_count)
		minimap_center = circle_bonfire_positions[circle_count];
	} else if (circle_bonfire_positions.size() > 0) {
		// Fallback to last known position
		minimap_center = circle_bonfire_positions.back();
	} else {
		// Final fallback
		minimap_center = spawn_position;
	}
	
	// Update all circles (current and previous ones)
	update_circles(circle_count, spawn_radius, circle_bonfire_positions);
	
	Motion& player_motion = registry.motions.get(player_entity);
	vec2 player_pos = player_motion.position;
	
	// Minimap shows minimap_world_view_radius pixels of world space
	// The most recent circle's center is at the center of the minimap
	float world_to_minimap_scale = MINIMAP_RADIUS / minimap_world_view_radius;
	
	// Calculate player position relative to the most recent circle's center
	vec2 player_relative_to_spawn = player_pos - minimap_center;
	
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
	
#else
	(void)player_entity;
	(void)spawn_radius;
	(void)spawn_position;
	(void)circle_count;
#endif
}

void MinimapSystem::update_bonfire_position(vec2 bonfire_world_pos, float spawn_radius, vec2 spawn_position)
{
#ifdef HAVE_RMLUI
	if (!minimap_document) {
		return;
	}
	
	// Scale minimap world view radius with spawn radius (same as in update_player_position)
	minimap_world_view_radius = spawn_radius * 1.25f;
	
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
	
	// Minimap shows minimap_world_view_radius pixels of world space
	// The spawn position is at the center of the minimap
	float world_to_minimap_scale = MINIMAP_RADIUS / minimap_world_view_radius;
	
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
	(void)spawn_radius;
	(void)spawn_position;
#endif
}

void MinimapSystem::update_circles(int circle_count, float current_spawn_radius, const std::vector<vec2>& circle_bonfire_positions)
{
#ifdef HAVE_RMLUI
	if (!minimap_document) {
		return;
	}
	
	Rml::Element* minimap_container = minimap_document->GetElementById("minimap_container");
	if (!minimap_container) {
		return;
	}
	
	// Get the most recent/biggest circle's center position (this is the minimap center)
	// Circle 0 uses initial spawn, Circle N (N > 0) uses bonfire from circle N-1
	vec2 current_spawn_pos;
	if (circle_bonfire_positions.size() == 0) {
		// No positions initialized yet - use origin as fallback
		current_spawn_pos = vec2(0.0f, 0.0f);
	} else if (circle_count == 0) {
		// Circle 0: center at initial spawn position
		current_spawn_pos = circle_bonfire_positions[0];
	} else if (circle_count < circle_bonfire_positions.size()) {
		// Circle N: center at bonfire from previous circle (stored at index circle_count)
		current_spawn_pos = circle_bonfire_positions[circle_count];
	} else {
		// Fallback to last known position
		current_spawn_pos = circle_bonfire_positions.back();
	}
	
	// Calculate how many circles we need to show (circle_count + 1, since circle_count is 0-indexed for the first circle)
	// Actually, circle_count represents how many circles have been completed, so we show circle_count + 1 circles total
	int total_circles = circle_count + 1;
	
	// World to minimap scale
	float world_to_minimap_scale = MINIMAP_RADIUS / minimap_world_view_radius;
	
	// Ensure we have enough circle elements
	// Create or update circles for each previous circle
	for (int i = 0; i < total_circles; i++) {
		// Calculate spawn radius for this circle
		float circle_spawn_radius = INITIAL_SPAWN_RADIUS + (i * RADIUS_INCREASE_PER_CIRCLE);
		
		// Get bonfire position for this circle
		vec2 circle_center;
		if (circle_bonfire_positions.size() == 0) {
			// No positions initialized - use origin
			circle_center = vec2(0.0f, 0.0f);
		} else if (i == 0) {
			// Circle 0 always uses the initial spawn position
			circle_center = circle_bonfire_positions[0];
		} else if (i < circle_bonfire_positions.size()) {
			// Circle i uses bonfire from circle i-1 (stored at index i)
			circle_center = circle_bonfire_positions[i];
		} else {
			// Fallback to current spawn if not available
			circle_center = current_spawn_pos;
		}
		
		// Calculate circle center position relative to current spawn (minimap center)
		vec2 circle_relative_to_spawn = circle_center - current_spawn_pos;
		float circle_center_x_minimap = circle_relative_to_spawn.x * world_to_minimap_scale;
		float circle_center_y_minimap = circle_relative_to_spawn.y * world_to_minimap_scale;
		
		// Get or create circle element
		char circle_id[64];
		snprintf(circle_id, sizeof(circle_id), "spawn_radius_%d", i);
		Rml::Element* circle = minimap_document->GetElementById(circle_id);
		
		if (!circle) {
			// Create new circle element if it doesn't exist
			Rml::ElementPtr circle_ptr = minimap_document->CreateElement("div");
			circle = circle_ptr.get();
			circle->SetId(circle_id);
			circle->SetProperty("position", "absolute");
			circle->SetProperty("background-color", "transparent");
			circle->SetProperty("border", "3px #FF9500"); // RmlUi syntax: border width color (no "solid")
			// Set z-index so older circles are behind newer ones
			char z_index_str[32];
			snprintf(z_index_str, sizeof(z_index_str), "%d", i);
			circle->SetProperty("z-index", z_index_str);
			minimap_container->AppendChild(std::move(circle_ptr));
		}
		
		// Calculate alpha based on age (newest = 1.0, older = decreasing)
		// Current circle (i == circle_count) has full opacity
		// Older circles have decreasing opacity
		float alpha = 1.0f;
		if (i < circle_count) {
			// Older circles: decrease alpha by 0.25 per circle
			// So if on circle 4 (circle_count = 3), then:
			// Circle 3 (i=3): alpha = 1.0 (current)
			// Circle 2 (i=2): alpha = 0.75 (slightly darker)
			// Circle 1 (i=1): alpha = 0.5 (darker)
			// Circle 0 (i=0): alpha = 0.25 (darkest)
			int age = circle_count - i;
			alpha = std::max(0.25f, 1.0f - (age * 0.25f));
		}
		
		// Scale the world spawn radius to minimap coordinates
		float circle_radius_minimap = (circle_spawn_radius / minimap_world_view_radius) * MINIMAP_RADIUS;
		// Clamp to ensure it doesn't exceed the minimap bounds
		circle_radius_minimap = std::min(circle_radius_minimap, MINIMAP_RADIUS * 0.95f);
		// Ensure minimum size so circles are visible
		circle_radius_minimap = std::max(circle_radius_minimap, 10.0f);
		float circle_diameter = circle_radius_minimap * 2.0f;
		float circle_half = circle_radius_minimap;
		
		// Calculate position: center of minimap + offset to circle center
		float circle_left = MINIMAP_RADIUS + circle_center_x_minimap - circle_half;
		float circle_top = MINIMAP_RADIUS + circle_center_y_minimap - circle_half;
		
		// Update circle properties
		char width_str[32];
		char height_str[32];
		char left_str[32];
		char top_str[32];
		char border_radius_str[32];
		char opacity_str[32];
		char z_index_str[32];
		snprintf(width_str, sizeof(width_str), "%.1fpx", circle_diameter);
		snprintf(height_str, sizeof(height_str), "%.1fpx", circle_diameter);
		snprintf(left_str, sizeof(left_str), "%.1fpx", circle_left);
		snprintf(top_str, sizeof(top_str), "%.1fpx", circle_top);
		snprintf(border_radius_str, sizeof(border_radius_str), "%.1fpx", circle_half);
		snprintf(opacity_str, sizeof(opacity_str), "%.2f", alpha);
		snprintf(z_index_str, sizeof(z_index_str), "%d", i);
		
		circle->SetProperty("width", width_str);
		circle->SetProperty("height", height_str);
		circle->SetProperty("left", left_str);
		circle->SetProperty("top", top_str);
		circle->SetProperty("border-radius", border_radius_str);
		circle->SetProperty("opacity", opacity_str);
		circle->SetProperty("z-index", z_index_str);
		circle->SetProperty("display", "block");
	}
	
	// Hide any extra circles that shouldn't be visible
	// (in case circle_count decreased, though this shouldn't happen in normal gameplay)
	for (int i = total_circles; i < 20; i++) { // Check up to 20 circles max
		char circle_id[64];
		snprintf(circle_id, sizeof(circle_id), "spawn_radius_%d", i);
		Rml::Element* circle = minimap_document->GetElementById(circle_id);
		if (circle) {
			circle->SetProperty("display", "none");
		}
	}
	
	// Hide the original spawn_radius element since we're using dynamic circles now
	Rml::Element* spawn_radius_circle = minimap_document->GetElementById("spawn_radius");
	if (spawn_radius_circle) {
		spawn_radius_circle->SetProperty("display", "none");
	}
#else
	(void)circle_count;
	(void)current_spawn_radius;
	(void)circle_bonfire_positions;
#endif
}

void MinimapSystem::render()
{
}

void MinimapSystem::set_visible(bool visible)
{
#ifdef HAVE_RMLUI
	if (!minimap_document) {
		return;
	}

	Rml::Element* container = minimap_document->GetElementById("minimap_container");
	if (!container) {
		return;
	}

	container->SetClass("hud-visible", visible);
	container->SetClass("hud-hidden", !visible);
#else
	(void)visible;
#endif
}

void MinimapSystem::play_intro_animation()
{
	set_visible(true);
}

