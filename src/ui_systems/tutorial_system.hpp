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
	std::string pointer_target;
	std::string pointer_position;
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
	
	void start_tutorial();
	void next_step();
	void skip_tutorial();
	bool is_active() const { return tutorial_active; }
	int get_current_step() const { return current_step; }
	bool should_pause() const { return tutorial_active && pause_gameplay; }
	
	enum class Action { None, Move, Shoot, OpenInventory, Reload };
	Action get_required_action() const { return required_action; }
	void notify_action(Action action);
	
	void on_mouse_move(vec2 mouse_position);
	void on_mouse_button(int button, int action, int mods);
	
	void on_next_clicked();
	void on_skip_clicked();

private:
	void update_tutorial_content();
	void setup_tutorial_steps();
	void start_tutorial_internal();
	void set_required_action(Action action);
	
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

	Action required_action = Action::None;
	bool awaiting_action = false;
	bool pause_gameplay = true;
	
	float action_completed_delay = 0.0f;
	bool waiting_for_delay = false;
	
	// For waiting for player to run out of ammo before showing reload tutorial
	bool waiting_for_out_of_ammo = false;
	float post_reload_delay = 7000.0f; // 7 seconds after reload
};