#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class HUDSystem
{
public:
	HUDSystem();
	~HUDSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif

	// Update the HUD data
	void update(float elapsed_ms);

	// Render the HUD
	void render();

	// Update HUD values from player data
	void update_player_stats(Entity player_entity);

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* hud_document = nullptr;
#endif

	// Track player entity
	Entity tracked_player;
};

