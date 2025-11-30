#include "tutorial_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>

class TutorialNextListener : public Rml::EventListener
{
public:
	TutorialNextListener(TutorialSystem* system) : tutorial_system(system) {}
	
	void ProcessEvent(Rml::Event& event) override {
		if (tutorial_system) {
			tutorial_system->on_next_clicked();
		}
	}
	
private:
	TutorialSystem* tutorial_system;
};

class TutorialSkipListener : public Rml::EventListener
{
public:
	TutorialSkipListener(TutorialSystem* system) : tutorial_system(system) {}
	
	void ProcessEvent(Rml::Event& event) override {
		if (tutorial_system) {
			tutorial_system->on_skip_clicked();
		}
	}
	
private:
	TutorialSystem* tutorial_system;
};

static TutorialNextListener* next_listener = nullptr;
static TutorialSkipListener* skip_listener = nullptr;
#endif

TutorialSystem::TutorialSystem()
	: current_step(0), tutorial_active(false), tutorial_completed(false), 
	  should_start_tutorial(false), tutorial_start_delay(0.0f)
{
	setup_tutorial_steps();
}

TutorialSystem::~TutorialSystem()
{
#ifdef HAVE_RMLUI
	// Note: Don't call tutorial_document->Close() here to avoid crash during RmlUI shutdown
	// RmlUI will clean up documents when Rml::Shutdown() is called in main.cpp
	if (next_listener) {
		delete next_listener;
		next_listener = nullptr;
	}
	if (skip_listener) {
		delete skip_listener;
		skip_listener = nullptr;
	}
#endif
}

#ifdef HAVE_RMLUI
bool TutorialSystem::init(Rml::Context* context)
#else
bool TutorialSystem::init(void* context)
#endif
{
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null" << std::endl;
		return false;
	}

	tutorial_document = rml_context->LoadDocument("ui/tutorial.rml");
	
	if (!tutorial_document) {
		std::cerr << "ERROR: Failed to load tutorial document" << std::endl;
		return false;
	}

	next_listener = new TutorialNextListener(this);
	skip_listener = new TutorialSkipListener(this);
	
	Rml::Element* next_button = tutorial_document->GetElementById("tutorial_next_btn");
	if (next_button) {
		next_button->AddEventListener(Rml::EventId::Click, next_listener);
	}
	
	Rml::Element* skip_button = tutorial_document->GetElementById("tutorial_skip_btn");
	if (skip_button) {
		skip_button->AddEventListener(Rml::EventId::Click, skip_listener);
	}

	tutorial_document->Hide();
	return true;
#else
	(void)context;
	std::cerr << "ERROR: RmlUi not available - Tutorial disabled" << std::endl;
	return false;
#endif
}

void TutorialSystem::setup_tutorial_steps()
{
	tutorial_steps = {
		{
			"Welcome to the Game!",
			"Use WASD keys to move your character around the world.",
			"",
			"none"
		},
		{
			"How to Shoot",
			"Left click to shoot your weapon. Aim with your mouse and click to fire!",
			"",
			"none"
		},
		{
			"How to Reload",
			"Press R to reload your weapon when you run low on ammo!",
			"",
			"none"
		},
		{
			"Health & Ammo",
			"Keep an eye on your Health and Ammo bars. Running out of either can be dangerous!",
			"stats",
			"right"
		},
		{
			"Minimap",
			"The minimap shows your surroundings. The yellow circle indicates the danger zone - move outside it to reach the safe zone!",
			"minimap",
			"left"
		},
		{
			"Currency",
			"Collect currency by defeating enemies. Use it to buy upgrades!",
			"currency",
			"bottom"
		},
		{
			"Objectives",
			"Check your objectives to know what to do next. Complete them to progress!",
			"objectives",
			"left"
		},
		{
			"Ready to Play!",
			"You're all set! Good luck and have fun exploring. Press Next to start your adventure!",
			"",
			"none"
		}
	};
}

void TutorialSystem::reset_tutorial()
{
	tutorial_completed = false;
	tutorial_active = false;
	current_step = 0;
	should_start_tutorial = false;
	tutorial_start_delay = 0.0f;
}

void TutorialSystem::start_tutorial()
{
#ifdef HAVE_RMLUI
	if (!tutorial_completed) {
		should_start_tutorial = true;
		tutorial_start_delay = 500.0f; // 500ms delay
	}
#endif
}

void TutorialSystem::start_tutorial_internal()
{
#ifdef HAVE_RMLUI
	if (!tutorial_completed && tutorial_document && rml_context) {
		tutorial_active = true;
		current_step = 0;
		pause_gameplay = true;
		
		tutorial_document->Show();
		
		Rml::Element* next_button = tutorial_document->GetElementById("tutorial_next_btn");
		if (next_button) {
			if (next_listener) {
				next_button->RemoveEventListener(Rml::EventId::Click, next_listener);
			}
			next_button->AddEventListener(Rml::EventId::Click, next_listener);
		}
		
		Rml::Element* skip_button = tutorial_document->GetElementById("tutorial_skip_btn");
		if (skip_button) {
			if (skip_listener) {
				skip_button->RemoveEventListener(Rml::EventId::Click, skip_listener);
			}
			skip_button->AddEventListener(Rml::EventId::Click, skip_listener);
		}
		
		update_tutorial_content();
	}
#endif
}

void TutorialSystem::next_step()
{
#ifdef HAVE_RMLUI
	if (!tutorial_active || !tutorial_document) {
		return;
	}

	current_step++;
	
	if (current_step >= (int)tutorial_steps.size()) {
		skip_tutorial();
	} else {
		update_tutorial_content();
	}
#endif
}

void TutorialSystem::skip_tutorial()
{
#ifdef HAVE_RMLUI
	if (tutorial_document) {
		tutorial_document->Hide();
	}
	tutorial_active = false;
	tutorial_completed = true;
#endif
}

void TutorialSystem::set_required_action(Action action)
{
	required_action = action;
	awaiting_action = false;
	pause_gameplay = true;
}

void TutorialSystem::notify_action(Action action)
{
	if (!tutorial_active) return;
	
	// Special case: if we're waiting for reload after running out of ammo
	if (waiting_for_out_of_ammo && action == Action::Reload) {
		waiting_for_out_of_ammo = false;
		waiting_for_delay = true;
		action_completed_delay = post_reload_delay; // 7 seconds
		if (tutorial_document) {
			tutorial_document->Hide();
		}
		return;
	}
	
	if (!awaiting_action) return;
	if (action == required_action) {
		awaiting_action = false;
		
		// Special case: after shoot action, don't advance yet - wait for out of ammo
		if (action == Action::Shoot) {
			waiting_for_out_of_ammo = true;
			waiting_for_delay = false;
			pause_gameplay = false;
			return;
		}
		
		waiting_for_delay = true;
		action_completed_delay = 3000.0f;
	}
}

void TutorialSystem::update_tutorial_content()
{
#ifdef HAVE_RMLUI
	
	if (!tutorial_document) {
		return;
	}
	if (!rml_context) {
		return;
	}
	if (current_step >= (int)tutorial_steps.size()) {
		return;
	}
	pause_gameplay = true;

	const TutorialStep& step = tutorial_steps[current_step];
	
	Rml::Element* title_elem = tutorial_document->GetElementById("tutorial_title");
	if (title_elem) {
		title_elem->SetInnerRML(step.title.c_str());
	}
	
	Rml::Element* desc_elem = tutorial_document->GetElementById("tutorial_description");
	if (desc_elem) {
		desc_elem->SetInnerRML(step.description.c_str());
	}
	
	Rml::Element* step_elem = tutorial_document->GetElementById("tutorial_step");
	if (step_elem) {
		char step_text[64];
		snprintf(step_text, sizeof(step_text), "Step %d of %d", current_step + 1, (int)tutorial_steps.size());
		step_elem->SetInnerRML(step_text);
	}
	
	Rml::Element* pointer_elem = tutorial_document->GetElementById("tutorial_pointer");
	if (pointer_elem) {
		if (step.pointer_position == "none" || step.pointer_target.empty()) {
			pointer_elem->SetProperty("display", "none");
		} else {
			pointer_elem->SetProperty("display", "block");
			std::string pointer_class = "tutorial_pointer pointer_" + step.pointer_position;
			pointer_elem->SetAttribute("class", pointer_class.c_str());
		}
	}
	
	Rml::Element* dialog_elem = tutorial_document->GetElementById("tutorial_dialog");
	if (dialog_elem) {
		std::string dialog_class;
		if (step.pointer_target == "stats") {
			dialog_class = "tutorial_dialog position_stats";
		} else if (step.pointer_target == "minimap") {
			dialog_class = "tutorial_dialog position_minimap";
		} else if (step.pointer_target == "currency") {
			dialog_class = "tutorial_dialog position_currency";
		} else if (step.pointer_target == "objectives") {
			dialog_class = "tutorial_dialog position_objectives";
		} else {
			dialog_class = "tutorial_dialog position_center";
		}
		dialog_elem->SetAttribute("class", dialog_class.c_str());
	}
	
	Rml::Element* next_btn = tutorial_document->GetElementById("tutorial_next_btn");
	if (next_btn) {
		if (current_step == (int)tutorial_steps.size() - 1) {
			next_btn->SetInnerRML("Start!");
		} else {
			next_btn->SetInnerRML("Next");
		}
	}

	set_required_action(Action::None);
	if (step.title.find("Welcome") != std::string::npos) {
		set_required_action(Action::Move);
	} else if (step.title.find("How to Shoot") != std::string::npos || step.title.find("Shoot") != std::string::npos) {
		set_required_action(Action::Shoot);
	} else if (step.title.find("How to Reload") != std::string::npos || step.title.find("Reload") != std::string::npos) {
		set_required_action(Action::Reload);
	}
#endif
}

void TutorialSystem::update(float elapsed_ms)
{
	if (should_start_tutorial && tutorial_start_delay > 0.0f) {
		tutorial_start_delay -= elapsed_ms;
		if (tutorial_start_delay <= 0.0f) {
			should_start_tutorial = false;
			start_tutorial_internal();
		}
	}
	
	// Check if player has run out of ammo during shoot tutorial
	if (waiting_for_out_of_ammo && tutorial_active) {
		// Access player to check ammo
		for (Entity entity : registry.players.entities) {
			Player& player = registry.players.get(entity);
			if (player.ammo_in_mag <= 0) {
				// Player is out of ammo, show reload tutorial
				waiting_for_out_of_ammo = false;
				pause_gameplay = true;
				if (tutorial_document) {
					tutorial_document->Show();
				}
				next_step(); // Move to reload tutorial
				break;
			}
		}
	}

	if (waiting_for_delay && action_completed_delay > 0.0f) {
		action_completed_delay -= elapsed_ms;
		if (action_completed_delay <= 0.0f) {
			waiting_for_delay = false;
			pause_gameplay = true;
			if (tutorial_document) {
				tutorial_document->Show();
			}
			next_step();
		}
	}
}

void TutorialSystem::render()
{
}

void TutorialSystem::on_next_clicked()
{
	if (required_action != Action::None && !awaiting_action) {
		awaiting_action = true;
		pause_gameplay = false;
		if (tutorial_document) {
			tutorial_document->Hide();
		}
		return;
	}
	next_step();
}

void TutorialSystem::on_skip_clicked()
{
	skip_tutorial();
}

void TutorialSystem::on_mouse_move(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (rml_context && tutorial_active) {
		rml_context->ProcessMouseMove((int)(mouse_position.x * 2), (int)(mouse_position.y * 2), 0);
	}
#else
	(void)mouse_position;
#endif
}

void TutorialSystem::on_mouse_button(int button, int action, int mods)
{
#ifdef HAVE_RMLUI
	if (rml_context && tutorial_active) {
		int rml_button = button;
		
		if (action == GLFW_PRESS) {
			rml_context->ProcessMouseButtonDown(rml_button, 0);
		} else if (action == GLFW_RELEASE) {
			rml_context->ProcessMouseButtonUp(rml_button, 0);
		}
	}
#else
	(void)button;
	(void)action;
	(void)mods;
#endif
}


