#include "stats_system.hpp"
#include "health_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

StatsSystem::StatsSystem()
{
}

StatsSystem::~StatsSystem()
{
#ifdef HAVE_RMLUI
	// Note: Don't call hud_document->Close() here to avoid crash during RmlUI shutdown
	// RmlUI will clean up documents when Rml::Shutdown() is called in main.cpp
#endif
}

#ifdef HAVE_RMLUI
bool StatsSystem::init(Rml::Context* context)
#else
bool StatsSystem::init(void* context)
#endif
{
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null" << std::endl;
		return false;
	}

	hud_document = rml_context->LoadDocument("ui/hud.rml");
	
	if (!hud_document) {
		std::cerr << "ERROR: Failed to load HUD document" << std::endl;
		return false;
	}

	hud_document->Show();
	set_visible(false);
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - HUD disabled (HAVE_RMLUI not defined)" << std::endl;
	return false;
#endif
}

void StatsSystem::update(float elapsed_ms)
{
	(void)elapsed_ms; // Suppress unused warning
	// HUD is always visible, no toggle needed
	// Stats are updated via update_player_stats()
}

void StatsSystem::render()
{
}

void StatsSystem::update_player_stats(Entity player_entity, HealthSystem* health_system)
{
#ifdef HAVE_RMLUI
	if (!hud_document || !registry.players.has(player_entity)) {
		return;
	}

	Player& player = registry.players.get(player_entity);
	
	// Calculate percentages - use health system if available, otherwise fall back to direct access
	float health_percent = 100.0f;
	if (health_system) {
		health_percent = health_system->get_health_percent(player_entity);
	} else {
		health_percent = (float)player.health / (float)player.max_health * 100.0f;
	}
	float ammo_percent = (float)player.ammo_in_mag / (float)player.magazine_size * 100.0f;
	
	// Clamp values
	health_percent = glm::clamp(health_percent, 0.0f, 100.0f);
	ammo_percent = glm::clamp(ammo_percent, 0.0f, 100.0f);
	
	// Update health bar
	Rml::Element* health_bar = hud_document->GetElementById("health_bar");
	if (health_bar) {
		char width_str[32];
		snprintf(width_str, sizeof(width_str), "%.1f%%", health_percent);
		health_bar->SetProperty("width", width_str);
	}
	
	// Update ammo bar
	Rml::Element* ammo_bar = hud_document->GetElementById("ammo_bar");
	if (ammo_bar) {
		char width_str[32];
		snprintf(width_str, sizeof(width_str), "%.1f%%", ammo_percent);
		ammo_bar->SetProperty("width", width_str);
	}
#else
	(void)player_entity; // Suppress unused warning
#endif
}

void StatsSystem::update_crosshair_ammo(Entity player_entity, vec2 mouse_pos)
{
#ifdef HAVE_RMLUI
	if (!hud_document || !registry.players.has(player_entity)) {
		return;
	}

	Player& player = registry.players.get(player_entity);
	
	// Get the ammo display element
	Rml::Element* ammo_display = hud_document->GetElementById("crosshair_ammo_display");
	Rml::Element* ammo_text = hud_document->GetElementById("crosshair_ammo_text");
	
	if (!ammo_display || !ammo_text) {
		return;
	}
	
	// Update ammo text
	char ammo_str[32];
	snprintf(ammo_str, sizeof(ammo_str), "%d", player.ammo_in_mag);
	ammo_text->SetInnerRML(ammo_str);
	
	ammo_text->SetProperty("color", "white");
	
	float screen_x = mouse_pos.x * 2.0f;
	float screen_y = mouse_pos.y * 2.0f;
	
	float offset_y = 45.0f;
	float offset_x = 30.0f;
	
	char pos_str[64];
	snprintf(pos_str, sizeof(pos_str), "%.0fpx", screen_x);
	ammo_display->SetProperty("left", pos_str);
	
	snprintf(pos_str, sizeof(pos_str), "%.0fpx", screen_y + offset_y + offset_x);
	ammo_display->SetProperty("top", pos_str);
	
	ammo_display->SetClass("visible", true);
#else
	(void)player_entity;
	(void)mouse_pos;
#endif
}

// Update reload progress bar
void StatsSystem::update_reload_bar(Entity player_entity, vec2 mouse_pos)
{
	if (!hud_document || !registry.sprites.has(player_entity)) {
		return;
	}

	// Get the reload bar elements
	Sprite& sprite = registry.sprites.get(player_entity);
	Rml::Element* container = hud_document->GetElementById("reload_bar_container");
	Rml::Element* fill = hud_document->GetElementById("reload_bar_fill");

	if (!container || !fill) {
		return;
	}

	// Don't show the reload bar if not reloading
	if (!sprite.is_reloading) {
		container->SetClass("visible", false);
		return;
	}

	// Calculate reload progress
	float progress = 1.0f - (sprite.reload_timer / sprite.reload_duration);
	if (progress < 0.0f) progress = 0.0f;
	if (progress > 1.0f) progress = 1.0f;

	// Update position and fill height
	char pos_str[64];
	snprintf(pos_str, sizeof(pos_str), "%.0fpx", mouse_pos.x * 2.0f + 40.0f);
	container->SetProperty("left", pos_str);
	snprintf(pos_str, sizeof(pos_str), "%.0fpx", mouse_pos.y * 2.0f - 20.0f);
	container->SetProperty("top", pos_str);

	snprintf(pos_str, sizeof(pos_str), "%.0f%%", progress * 100.0f);
	fill->SetProperty("height", pos_str);

	// Show the reload bar
	container->SetClass("visible", true);

	(void)player_entity;
	(void)mouse_pos;
}

void StatsSystem::set_visible(bool visible)
{
#ifdef HAVE_RMLUI
	if (!hud_document) {
		return;
	}

	Rml::Element* container = hud_document->GetElementById("hud_container");
	if (!container) {
		return;
	}

	container->SetClass("hud-visible", visible);
	container->SetClass("hud-hidden", !visible);
#else
	(void)visible;
#endif
}

void StatsSystem::play_intro_animation()
{
	set_visible(true);
}

void StatsSystem::set_ammo_counter_opacity(float opacity)
{
#ifdef HAVE_RMLUI
	if (!hud_document) {
		return;
	}

	Rml::Element* ammo_display = hud_document->GetElementById("crosshair_ammo_display");
	if (!ammo_display) {
		return;
	}

	// Clamp opacity to valid range
	if (opacity < 0.0f) opacity = 0.0f;
	if (opacity > 1.0f) opacity = 1.0f;

	// Set opacity using CSS property
	char opacity_str[32];
	snprintf(opacity_str, sizeof(opacity_str), "%.2f", opacity);
	ammo_display->SetProperty("opacity", opacity_str);
#else
	(void)opacity;
#endif
}

