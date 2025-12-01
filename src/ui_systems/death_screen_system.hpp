#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class DeathScreenSystem
{
public:
	DeathScreenSystem();
	~DeathScreenSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif

	// Update the death screen animation
	void update(float elapsed_ms);

	// Render the death screen
	void render();

	// Show the death screen with animation
	void show();

	// Hide the death screen
	void hide();

	// Check if death screen is visible
	bool is_visible() const { return is_death_screen_visible; }

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* death_screen_document = nullptr;
#endif

	bool is_death_screen_visible = false;
	float animation_time = 0.0f;
	const float ANIMATION_DURATION = 1.5f; // 1.5 seconds for the pop animation
};

