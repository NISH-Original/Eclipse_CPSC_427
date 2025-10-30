#include "tutorial_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>

// Event listener for the Next button
class TutorialNextListener : public Rml::EventListener
{
public:
	TutorialNextListener(TutorialSystem* system) : tutorial_system(system) {}
	
	void ProcessEvent(Rml::Event& event) override {
		printf("TutorialNextListener::ProcessEvent - NEXT BUTTON CLICKED!\n");
		fflush(stdout);
		if (tutorial_system) {
			tutorial_system->on_next_clicked();
		}
	}
	
private:
	TutorialSystem* tutorial_system;
};

// Event listener for the Skip button
class TutorialSkipListener : public Rml::EventListener
{
public:
	TutorialSkipListener(TutorialSystem* system) : tutorial_system(system) {}
	
	void ProcessEvent(Rml::Event& event) override {
		printf("TutorialSkipListener::ProcessEvent - SKIP BUTTON CLICKED!\n");
		fflush(stdout);
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
	if (tutorial_document) {
		tutorial_document->Close();
	}
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

	// Setup event listeners
	next_listener = new TutorialNextListener(this);
	skip_listener = new TutorialSkipListener(this);
	
	printf("TutorialSystem::init - Looking for buttons...\n");
	fflush(stdout);
	
	Rml::Element* next_button = tutorial_document->GetElementById("tutorial_next_btn");
	if (next_button) {
		printf("TutorialSystem::init - Found next button, adding listener\n");
		fflush(stdout);
		next_button->AddEventListener(Rml::EventId::Click, next_listener);
	} else {
		printf("TutorialSystem::init - ERROR: next button NOT FOUND\n");
		fflush(stdout);
	}
	
	Rml::Element* skip_button = tutorial_document->GetElementById("tutorial_skip_btn");
	if (skip_button) {
		printf("TutorialSystem::init - Found skip button, adding listener\n");
		fflush(stdout);
		skip_button->AddEventListener(Rml::EventId::Click, skip_listener);
	} else {
		printf("TutorialSystem::init - ERROR: skip button NOT FOUND\n");
		fflush(stdout);
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
			"Use WASD keys to move your character around the world. Left click to shoot your weapon.",
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
			"The minimap in the top-right shows your surroundings. Use it to navigate and spot enemies!",
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
			"Inventory",
			"Press I to open your inventory. Equip weapons and armor to improve your stats.",
			"",
			"none"
		},
		{
			"Objectives",
			"Check your objectives to know what to do next. Complete them to progress!",
			"objectives",
			"left"
		},
		{
			"Combat Tips",
			"Aim with your mouse and click to shoot. Keep moving to avoid enemy attacks!",
			"",
			"none"
		},
		{
			"Ready to Play!",
			"You're all set! Good luck and have fun exploring. Press Next to start your adventure!",
			"",
			"none"
		}
	};
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
		
		printf("TutorialSystem::start_tutorial_internal - Showing document\n");
		fflush(stdout);
		tutorial_document->Show();
		
		printf("TutorialSystem::start_tutorial_internal - Re-attaching event listeners\n");
		fflush(stdout);
		
		// Re-attach event listeners now that document is shown
		Rml::Element* next_button = tutorial_document->GetElementById("tutorial_next_btn");
		if (next_button) {
			printf("TutorialSystem::start_tutorial_internal - Found next button\n");
			fflush(stdout);
			// Remove old listener if it exists
			if (next_listener) {
				next_button->RemoveEventListener(Rml::EventId::Click, next_listener);
			}
			next_button->AddEventListener(Rml::EventId::Click, next_listener);
		} else {
			printf("TutorialSystem::start_tutorial_internal - ERROR: Next button NOT FOUND\n");
			fflush(stdout);
		}
		
		Rml::Element* skip_button = tutorial_document->GetElementById("tutorial_skip_btn");
		if (skip_button) {
			printf("TutorialSystem::start_tutorial_internal - Found skip button\n");
			fflush(stdout);
			// Remove old listener if it exists
			if (skip_listener) {
				skip_button->RemoveEventListener(Rml::EventId::Click, skip_listener);
			}
			skip_button->AddEventListener(Rml::EventId::Click, skip_listener);
		} else {
			printf("TutorialSystem::start_tutorial_internal - ERROR: Skip button NOT FOUND\n");
			fflush(stdout);
		}
		
		printf("TutorialSystem::start_tutorial_internal - Updating content\n");
		fflush(stdout);
		update_tutorial_content();
		printf("TutorialSystem::start_tutorial_internal - Complete\n");
		fflush(stdout);
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
		// Tutorial complete
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

	const TutorialStep& step = tutorial_steps[current_step];
	
	// Update title
	Rml::Element* title_elem = tutorial_document->GetElementById("tutorial_title");
	if (title_elem) {
		title_elem->SetInnerRML(step.title.c_str());
	}
	
	// Update description
	Rml::Element* desc_elem = tutorial_document->GetElementById("tutorial_description");
	if (desc_elem) {
		desc_elem->SetInnerRML(step.description.c_str());
	}
	
	// Update step counter
	Rml::Element* step_elem = tutorial_document->GetElementById("tutorial_step");
	if (step_elem) {
		char step_text[64];
		snprintf(step_text, sizeof(step_text), "Step %d of %d", current_step + 1, (int)tutorial_steps.size());
		step_elem->SetInnerRML(step_text);
	}
	
	// Update pointer visibility and position
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
	
	// Update dialog position based on pointer target
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
	
	// Update button text for last step
	Rml::Element* next_btn = tutorial_document->GetElementById("tutorial_next_btn");
	if (next_btn) {
		if (current_step == (int)tutorial_steps.size() - 1) {
			next_btn->SetInnerRML("Start!");
		} else {
			next_btn->SetInnerRML("Next");
		}
	}
#endif
}

void TutorialSystem::update(float elapsed_ms)
{
	// Handle delayed tutorial start
	if (should_start_tutorial && tutorial_start_delay > 0.0f) {
		tutorial_start_delay -= elapsed_ms;
		if (tutorial_start_delay <= 0.0f) {
			should_start_tutorial = false;
			start_tutorial_internal();
		}
	}
}

void TutorialSystem::render()
{
	// Rendering is handled by inventory system's rml_context->Render()
	// which renders all RmlUi documents at once
}

void TutorialSystem::on_next_clicked()
{
	printf("TutorialSystem::on_next_clicked - Called!\n");
	fflush(stdout);
	next_step();
}

void TutorialSystem::on_skip_clicked()
{
	printf("TutorialSystem::on_skip_clicked - Called!\n");
	fflush(stdout);
	skip_tutorial();
}

void TutorialSystem::on_mouse_move(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (rml_context && tutorial_active) {
		printf("TutorialSystem::on_mouse_move - pos=(%.1f, %.1f), scaled=(%d, %d)\n", 
			mouse_position.x, mouse_position.y, 
			(int)(mouse_position.x * 2), (int)(mouse_position.y * 2));
		fflush(stdout);
		rml_context->ProcessMouseMove((int)(mouse_position.x * 2), (int)(mouse_position.y * 2), 0);
	}
#else
	(void)mouse_position;
#endif
}

void TutorialSystem::on_mouse_button(int button, int action, int mods)
{
#ifdef HAVE_RMLUI
	printf("TutorialSystem::on_mouse_button - button=%d, action=%d, active=%d, context=%p\n", 
		button, action, tutorial_active, (void*)rml_context);
	fflush(stdout);
	
	if (rml_context && tutorial_active) {
		int rml_button = button;
		
		if (action == GLFW_PRESS) {
			printf("TutorialSystem::on_mouse_button - Processing PRESS\n");
			fflush(stdout);
			bool result = rml_context->ProcessMouseButtonDown(rml_button, 0);
			printf("TutorialSystem::on_mouse_button - ProcessMouseButtonDown returned %d\n", result);
			fflush(stdout);
		} else if (action == GLFW_RELEASE) {
			printf("TutorialSystem::on_mouse_button - Processing RELEASE\n");
			fflush(stdout);
			bool result = rml_context->ProcessMouseButtonUp(rml_button, 0);
			printf("TutorialSystem::on_mouse_button - ProcessMouseButtonUp returned %d\n", result);
			fflush(stdout);
		}
	}
#else
	(void)button;
	(void)action;
	(void)mods;
#endif
}


