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
	set_visible(false);
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
	
	// Update spawn radius circle size based on actual spawn radius
	// Scale the world spawn radius to minimap coordinates
	float spawn_radius_minimap = spawn_radius * world_to_minimap_scale;
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

