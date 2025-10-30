#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include <vector>
#include <string>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

struct TutorialStep {
	std::string title;
	std::string description;
	std::string pointer_target; // empty, "stats", "minimap", "inventory", "currency", "objectives"
	std::string pointer_position; // "left", "right", "top", "bottom", "none"
};

class TutorialSystem
{
public:
	TutorialSystem();
	~TutorialSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif

	void update(float elapsed_ms);
	void render();
	
	// Tutorial control
	void start_tutorial();
	void next_step();
	void skip_tutorial();
	bool is_active() const { return tutorial_active; }
	int get_current_step() const { return current_step; }
	
	// Input event handling
	void on_mouse_move(vec2 mouse_position);
	void on_mouse_button(int button, int action, int mods);
	
	// Event handling
	void on_next_clicked();
	void on_skip_clicked();

private:
	void update_tutorial_content();
	void setup_tutorial_steps();
	void start_tutorial_internal();
	
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* tutorial_document = nullptr;
#endif

	std::vector<TutorialStep> tutorial_steps;
	int current_step;
	bool tutorial_active;
	bool tutorial_completed;
	bool should_start_tutorial;
	float tutorial_start_delay;
};

