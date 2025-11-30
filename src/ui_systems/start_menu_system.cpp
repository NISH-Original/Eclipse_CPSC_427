#include "start_menu_system.hpp"
#include "audio_system.hpp"

#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

StartMenuSystem::StartMenuSystem()
{
}

StartMenuSystem::~StartMenuSystem()
{
#ifdef HAVE_RMLUI
	// Note: Don't call start_menu_document->Close() here to avoid crash during RmlUI shutdown
	// RmlUI will clean up documents when Rml::Shutdown() is called in main.cpp
	start_menu_document = nullptr;
	root_container = nullptr;
#endif
}

#ifdef HAVE_RMLUI
bool StartMenuSystem::init(Rml::Context* context, AudioSystem* audio)
#else
bool StartMenuSystem::init(void* context, AudioSystem* audio)
#endif
{
	audio_system = audio;
	menu_supported = false;

#ifdef HAVE_RMLUI
	rml_context = context;
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null for start menu" << std::endl;
		return false;
	}

	start_menu_document = rml_context->LoadDocument("ui/start_menu.rml");
	if (!start_menu_document) {
		std::cerr << "ERROR: Failed to load start menu document" << std::endl;
		return false;
	}

	root_container = start_menu_document->GetElementById("start-menu");
	if (!root_container) {
		std::cerr << "ERROR: start_menu.rml is missing root element with id 'start-menu'" << std::endl;
		return false;
	}

	attach_listeners();
	start_menu_document->Hide();
	menu_supported = true;
#endif

	menu_visible = false;
	menu_exiting = false;
	return true;
}

void StartMenuSystem::show()
{
#ifdef HAVE_RMLUI
	if (!menu_supported || !start_menu_document || !root_container) {
		return;
	}

	if (!menu_visible) {
		start_menu_document->Show();
		start_menu_document->PullToFront();
		root_container->SetClass("exiting", false);
		root_container->SetClass("hidden", false);
		root_container->SetProperty("pointer-events", "auto");
		menu_visible = true;
		menu_exiting = false;
	}
#endif
}

void StartMenuSystem::update_continue_button(bool has_save_file)
{
#ifdef HAVE_RMLUI
	if (!menu_supported || !start_menu_document) {
		return;
	}

	Rml::Element* continue_button = start_menu_document->GetElementById("continue_button");
	if (continue_button) {
		if (has_save_file) {
			continue_button->SetClass("disabled", false);
			continue_button->SetProperty("pointer-events", "auto");
		} else {
			continue_button->SetClass("disabled", true);
			continue_button->SetProperty("pointer-events", "none");
		}
	}
#endif
}

void StartMenuSystem::hide_immediately()
{
#ifdef HAVE_RMLUI
	if (!menu_supported || !start_menu_document || !root_container) {
		return;
	}

	root_container->SetClass("exiting", false);
	root_container->SetClass("hidden", true);
	start_menu_document->Hide();
#endif

	bool was_visible = menu_visible;
	menu_visible = false;
	menu_exiting = false;
	
	// If menu was visible and we have a callback, trigger it since transitionend isn't firing
	if (was_visible && on_menu_hidden) {
		on_menu_hidden();
	}
}

void StartMenuSystem::begin_exit_sequence()
{
#ifdef HAVE_RMLUI
	if (!menu_supported || !start_menu_document || !root_container || !menu_visible || menu_exiting) {
		return;
	}

	menu_exiting = true;
	root_container->SetClass("exiting", true);
	root_container->SetProperty("pointer-events", "none");
	start_menu_document->PullToFront();
#else
	menu_exiting = true;
#endif
}

void StartMenuSystem::on_mouse_move(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (!menu_supported || !rml_context || !menu_visible) {
		return;
	}

	rml_context->ProcessMouseMove((int)(mouse_position.x * 2.f), (int)(mouse_position.y * 2.f), 0);
	update_pointer_state(mouse_position);
#else
	(void)mouse_position;
#endif
}

bool StartMenuSystem::on_mouse_button(int button, int action, int mods)
{
#ifdef HAVE_RMLUI
	(void)mods;
	if (!menu_supported || !rml_context || !menu_visible) {
		return false;
	}

	if (button != GLFW_MOUSE_BUTTON_LEFT) {
		return false;
	}

	if (action == GLFW_PRESS) {
		rml_context->ProcessMouseButtonDown(button, 0);
		pointer_pressed_on_button = pointer_over_button;
		return pointer_over_button;
	}

	if (action == GLFW_RELEASE) {
		rml_context->ProcessMouseButtonUp(button, 0);
		bool consumed = pointer_pressed_on_button;
		pointer_pressed_on_button = false;
		return consumed;
	}

	return false;
#else
	(void)button;
	(void)action;
	(void)mods;
	return false;
#endif
}

bool StartMenuSystem::on_key(int key, int action, int mods)
{
#ifdef HAVE_RMLUI
	(void)mods;
	if (!menu_supported || !menu_visible || action != GLFW_PRESS) {
		return false;
	}

	if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) {
		if (on_start_game) {
			on_start_game();
		}
		begin_exit_sequence();
		return true;
	}

	if (key == GLFW_KEY_ESCAPE) {
		if (on_continue_game) {
			on_continue_game();
		}
		begin_exit_sequence();
		return true;
	}

	return false;
#else
	(void)key;
	(void)action;
	(void)mods;
	return false;
#endif
}

#ifdef HAVE_RMLUI
void StartMenuSystem::ProcessEvent(Rml::Event& event)
{
	if (!menu_supported || !start_menu_document || !root_container) {
		return;
	}

	const Rml::Element* target = event.GetTargetElement();
	if (!target) {
		return;
	}

	if (event.GetId() == Rml::EventId::Click) {
		const Rml::String& id = target->GetId();
		if (id == "start_button") {
			if (on_start_game) {
				on_start_game();
			}
			begin_exit_sequence();
		} else if (id == "continue_button") {
			if (on_continue_game) {
				on_continue_game();
			} else if (on_start_game) {
				on_start_game();
			}
			begin_exit_sequence();
		} else if (id == "exit_button") {
			if (on_exit) {
				on_exit();
			}
		}
	} else if (event.GetId() == Rml::EventId::Transitionend) {
		if (menu_exiting && target == root_container) {
			start_menu_document->Hide();
			menu_visible = false;
			menu_exiting = false;
			if (on_menu_hidden) {
				on_menu_hidden();
			}
		}
	}
}

void StartMenuSystem::attach_listeners()
{
	if (!start_menu_document) {
		return;
	}

	const char* button_ids[] = {
		"continue_button",
		"start_button",
		"exit_button"
	};

	for (const char* id : button_ids) {
		if (Rml::Element* element = start_menu_document->GetElementById(id)) {
			element->AddEventListener(Rml::EventId::Click, this);
		}
	}

	if (root_container) {
		root_container->AddEventListener(Rml::EventId::Transitionend, this);
	}
}

void StartMenuSystem::update_pointer_state(vec2 mouse_position)
{
	if (!menu_supported || !rml_context) {
		return;
	}

	Rml::Element* hover = rml_context->GetHoverElement();
	bool over_button = false;
	while (hover) {
		Rml::String id = hover->GetId();
		if (id == "continue_button" ||
			id == "start_button" ||
			id == "tutorial_button" ||
			id == "settings_button" ||
			id == "exit_button") {
			over_button = true;
			break;
		}
		hover = hover->GetParentNode();
	}

	if (over_button != pointer_over_button) {
		pointer_over_button = over_button;
	}

	(void)mouse_position;
}
#endif

