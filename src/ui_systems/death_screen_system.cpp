#include "death_screen_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>
#include <cmath>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

DeathScreenSystem::DeathScreenSystem()
{
}

DeathScreenSystem::~DeathScreenSystem()
{
#ifdef HAVE_RMLUI
	// Note: Don't call death_screen_document->Close() here to avoid crash during RmlUI shutdown
	// RmlUI will clean up documents when Rml::Shutdown() is called in main.cpp
#endif
}

#ifdef HAVE_RMLUI
bool DeathScreenSystem::init(Rml::Context* context)
#else
bool DeathScreenSystem::init(void* context)
#endif
{
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null for death screen" << std::endl;
		return false;
	}

	death_screen_document = rml_context->LoadDocument("ui/death_screen.rml");
	
	if (!death_screen_document) {
		std::cerr << "ERROR: Failed to load death screen document" << std::endl;
		return false;
	}

	// Hide the document initially
	death_screen_document->Hide();
	hide();
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - Death screen disabled" << std::endl;
	return false;
#endif
}

void DeathScreenSystem::update(float elapsed_ms)
{
#ifdef HAVE_RMLUI
	if (!is_death_screen_visible || !death_screen_document) {
		return;
	}

	// Update animation time
	animation_time += elapsed_ms;
	
	// Clamp animation time to duration
	if (animation_time > ANIMATION_DURATION) {
		animation_time = ANIMATION_DURATION;
	}

	// Calculate animation progress (0.0 to 1.0)
	float progress = animation_time / ANIMATION_DURATION;
	
	// Use an easing function for smooth pop animation (ease-out cubic)
	// This creates a slow start that accelerates, then slows down
	float eased_progress = 1.0f - pow(1.0f - progress, 3.0f);
	
	// Calculate scale (starts at 0.3, ends at 1.0)
	float scale = 0.3f + (eased_progress * 0.7f);
	
	// Update the game over text element
	Rml::Element* game_over_text = death_screen_document->GetElementById("game_over_text");
	if (game_over_text) {
		char transform_str[64];
		snprintf(transform_str, sizeof(transform_str), "scale(%.3f)", scale);
		game_over_text->SetProperty("transform", transform_str);
		
		// Also update opacity (fade in)
		float opacity = progress; // Fade in from 0 to 1
		char opacity_str[32];
		snprintf(opacity_str, sizeof(opacity_str), "%.3f", opacity);
		game_over_text->SetProperty("opacity", opacity_str);
	}
#else
	(void)elapsed_ms;
#endif
}

void DeathScreenSystem::render()
{
	// Nothing to render, RmlUI handles it
}

void DeathScreenSystem::show()
{
#ifdef HAVE_RMLUI
	if (!death_screen_document) {
		return;
	}

	// Reset animation
	animation_time = 0.0f;
	is_death_screen_visible = true;

	// Ensure document is shown
	if (!death_screen_document->IsVisible()) {
		death_screen_document->Show();
	}

	// Show the container
	Rml::Element* container = death_screen_document->GetElementById("death_screen_container");
	if (container) {
		container->SetClass("visible", true);
		container->SetClass("hidden", false);
	}

	// Reset transform and opacity for animation
	Rml::Element* game_over_text = death_screen_document->GetElementById("game_over_text");
	if (game_over_text) {
		game_over_text->SetProperty("transform", "scale(0.3)");
		game_over_text->SetProperty("opacity", "0");
	}
#endif
}

void DeathScreenSystem::hide()
{
#ifdef HAVE_RMLUI
	if (!death_screen_document) {
		return;
	}

	is_death_screen_visible = false;
	animation_time = 0.0f;

	// Hide the document
	if (death_screen_document->IsVisible()) {
		death_screen_document->Hide();
	}

	Rml::Element* container = death_screen_document->GetElementById("death_screen_container");
	if (container) {
		container->SetClass("visible", false);
		container->SetClass("hidden", true);
	}
#endif
}

