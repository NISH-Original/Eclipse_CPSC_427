#include "menu_icons_system.hpp"
#include "audio_system.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

MenuIconsSystem::MenuIconsSystem()
{
}

MenuIconsSystem::~MenuIconsSystem()
{
#ifdef HAVE_RMLUI
	if (menu_icons_document) {
		menu_icons_document->Close();
	}
#endif
}

#ifdef HAVE_RMLUI
bool MenuIconsSystem::init(Rml::Context* context, AudioSystem* audio)
#else
bool MenuIconsSystem::init(void* context, AudioSystem* audio)
#endif
{
	audio_system = audio;
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null for menu icons" << std::endl;
		return false;
	}

	// Load the menu icons document
	menu_icons_document = rml_context->LoadDocument("ui/menu_icons.rml");
	
	if (!menu_icons_document) {
		std::cerr << "ERROR: Failed to load menu icons document" << std::endl;
		return false;
	}

	if (Rml::Element* sound_icon = menu_icons_document->GetElementById("sound_icon")) {
		sound_icon->AddEventListener(Rml::EventId::Click, this);
	}

	update_sound_icon();
	menu_icons_document->Show();
	set_visible(false);
	return true;
#else
	(void)context; // Suppress unused warning
	if (!audio_system) {
		std::cerr << "ERROR: Audio system is null - unable to control sound icon" << std::endl;
	}
	std::cerr << "ERROR: RmlUi not available - Menu Icons disabled" << std::endl;
	return false;
#endif
}

void MenuIconsSystem::render()
{
}

void MenuIconsSystem::set_visible(bool visible)
{
#ifdef HAVE_RMLUI
	if (!menu_icons_document) {
		return;
	}

	Rml::Element* container = menu_icons_document->GetElementById("menu_icons_container");
	if (!container) {
		return;
	}

	container->SetClass("hud-visible", visible);
	container->SetClass("hud-hidden", !visible);
	if (!visible) {
		pointer_over_menu_icon = false;
		pointer_down_on_icon = false;
	}
#else
	(void)visible;
#endif
}

void MenuIconsSystem::play_intro_animation()
{
	set_visible(true);
}

#ifdef HAVE_RMLUI
void MenuIconsSystem::ProcessEvent(Rml::Event& event)
{
	if (!menu_icons_document || !audio_system) {
		return;
	}

	if (event.GetId() != Rml::EventId::Click) {
		return;
	}

	Rml::Element* target = event.GetTargetElement();
	if (!target) {
		return;
	}

	if (target->GetId() == "sound_icon") {
		audio_system->toggle_muted();
		update_sound_icon();
	}
}

void MenuIconsSystem::update_sound_icon()
{
	if (!menu_icons_document) {
		return;
	}

	Rml::Element* sound_icon = menu_icons_document->GetElementById("sound_icon");
	if (!sound_icon) {
		return;
	}

	if (audio_system && audio_system->is_muted()) {
		sound_icon->SetInnerRML("🔇");
		sound_icon->SetAttribute("title", "Unmute sound");
	} else {
		sound_icon->SetInnerRML("♪");
		sound_icon->SetAttribute("title", "Mute sound");
	}
}
#endif

void MenuIconsSystem::on_mouse_move(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (rml_context) {
		last_mouse_position = Rml::Vector2f((float)(mouse_position.x * 2.0f), (float)(mouse_position.y * 2.0f));
		rml_context->ProcessMouseMove((int)last_mouse_position.x, (int)last_mouse_position.y, 0);
		pointer_over_menu_icon = is_mouse_over_menu_icon();
	}
#else
	(void)mouse_position;
#endif
}

bool MenuIconsSystem::on_mouse_button(int button, int action, int mods)
{
#ifdef HAVE_RMLUI
	(void)mods;
	if (!rml_context) {
		return false;
	}

	if (button != GLFW_MOUSE_BUTTON_LEFT) {
		return false;
	}

	// Update hover state in case we didn't get a move event beforehand.
	pointer_over_menu_icon = is_mouse_over_menu_icon();

	if (action == GLFW_PRESS) {
		if (pointer_over_menu_icon) {
			rml_context->ProcessMouseButtonDown(button, 0);
			pointer_down_on_icon = true;
			return true;
		}
	} else if (action == GLFW_RELEASE) {
		if (pointer_down_on_icon || pointer_over_menu_icon) {
			rml_context->ProcessMouseButtonUp(button, 0);
			pointer_down_on_icon = false;
			return true;
		}
	}

	return false;
#else
	(void)button;
	(void)action;
	(void)mods;
	return false;
#endif
}

#ifdef HAVE_RMLUI
bool MenuIconsSystem::is_mouse_over_menu_icon() const
{
	if (!menu_icons_document || !rml_context) {
		return false;
	}

	Rml::Element* element = rml_context->GetHoverElement();
	while (element) {
		Rml::String id = element->GetId();
		if (id == "sound_icon" || id == "settings_icon" || id == "exit_icon") {
			return true;
		}

		element = element->GetParentNode();
	}

	return false;
}
#endif

