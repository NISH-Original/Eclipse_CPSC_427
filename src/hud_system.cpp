#include "hud_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

HUDSystem::HUDSystem()
{
}

HUDSystem::~HUDSystem()
{
#ifdef HAVE_RMLUI
	if (hud_document) {
		hud_document->Close();
	}
#endif
}

#ifdef HAVE_RMLUI
bool HUDSystem::init(Rml::Context* context)
#else
bool HUDSystem::init(void* context)
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
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - HUD disabled (HAVE_RMLUI not defined)" << std::endl;
	return false;
#endif
}

void HUDSystem::update(float elapsed_ms)
{
	(void)elapsed_ms; // Suppress unused warning
	// HUD is always visible, no toggle needed
	// Stats are updated via update_player_stats()
}

void HUDSystem::render()
{
#ifdef HAVE_RMLUI
	if (rml_context) {
		rml_context->Update();
	}
#endif
}

void HUDSystem::update_player_stats(Entity player_entity)
{
#ifdef HAVE_RMLUI
	if (!hud_document || !registry.players.has(player_entity)) {
		return;
	}

	Player& player = registry.players.get(player_entity);
	
	// Calculate percentages
	float health_percent = (float)player.health / (float)player.max_health * 100.0f;
	float heat_percent = (float)player.heat / (float)player.max_heat * 100.0f;
	
	// Clamp values
	health_percent = glm::clamp(health_percent, 0.0f, 100.0f);
	heat_percent = glm::clamp(heat_percent, 0.0f, 100.0f);
	
	// Update health bar
	Rml::Element* health_bar = hud_document->GetElementById("health_bar");
	if (health_bar) {
		char width_str[32];
		snprintf(width_str, sizeof(width_str), "%.1f%%", health_percent);
		health_bar->SetProperty("width", width_str);
	}
	
	// Update heat bar
	Rml::Element* heat_bar = hud_document->GetElementById("heat_bar");
	if (heat_bar) {
		char width_str[32];
		snprintf(width_str, sizeof(width_str), "%.1f%%", heat_percent);
		heat_bar->SetProperty("width", width_str);
	}
#else
	(void)player_entity; // Suppress unused warning
#endif
}

