#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class StatsSystem
{
public:
	StatsSystem();
	~StatsSystem();
	
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

	// Update crosshair ammo display position and text
	void update_crosshair_ammo(Entity player_entity, vec2 mouse_pos);

	void play_intro_animation();
	void set_visible(bool visible);

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* hud_document = nullptr;
#endif

	// Track player entity
	Entity tracked_player;
};

