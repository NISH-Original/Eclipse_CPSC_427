#include "menu_icons_system.hpp"
#include "audio_system.hpp"
#include <iostream>
#include <functional>

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
	// Also add listener to the image element itself
	if (Rml::Element* sound_icon_img = menu_icons_document->GetElementById("sound_icon_img")) {
		sound_icon_img->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* settings_icon = menu_icons_document->GetElementById("settings_icon")) {
		settings_icon->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* exit_icon = menu_icons_document->GetElementById("exit_icon")) {
		exit_icon->AddEventListener(Rml::EventId::Click, this);
	}

	// Don't call update_sound_icon() here - audio system might not be initialized yet
	// It will be called when the HUD becomes visible
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
	// Update sound icon when HUD becomes visible (audio should be initialized by now)
	if (audio_system) {
		update_sound_icon();
	}
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

	// Walk up the parent chain to find the icon container (in case we clicked on a child element like an img)
	Rml::Element* icon_element = target;
	int depth = 0;
	while (icon_element && depth < 10) {
		Rml::String id = icon_element->GetId();
		
		if (id == "sound_icon" || id == "sound_icon_img") {
			if (audio_system) {
				audio_system->toggle_muted();
				update_sound_icon();
			}
			return;
		} else if (id == "settings_icon") {
			// TODO: Open settings menu
			// For now, do nothing
			return;
		} else if (id == "exit_icon" || id == "exit_icon_img") {
			if (on_return_to_menu) {
				on_return_to_menu();
			}
			return;
		}
		Rml::Element* parent = icon_element->GetParentNode();
		if (parent == icon_element || parent == nullptr) {
			// Prevent infinite loop if GetParentNode() returns itself or null
			break;
		}
		icon_element = parent;
		depth++;
	}
}

void MenuIconsSystem::update_sound_icon()
{
	if (!menu_icons_document) {
		return;
	}

	Rml::Element* sound_icon_img = menu_icons_document->GetElementById("sound_icon_img");
	if (!sound_icon_img) {
		return;
	}

	Rml::Element* sound_icon = menu_icons_document->GetElementById("sound_icon");
	if (!sound_icon) {
		return;
	}

	if (audio_system && audio_system->is_muted()) {
		sound_icon_img->SetAttribute("src", "../data/textures/sound-off.png");
		sound_icon->SetAttribute("title", "Unmute sound");
	} else {
		sound_icon_img->SetAttribute("src", "../data/textures/sound-on.png");
		sound_icon->SetAttribute("title", "Mute sound");
	}
}
#endif

void MenuIconsSystem::on_mouse_move(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (rml_context) {
		// mouse_position is in window coordinates, but RmlUi context is created with framebuffer dimensions
		// On retina displays, framebuffer is 2x window size, so we need to scale coordinates
		last_mouse_position = Rml::Vector2f(mouse_position.x * 2.0f, mouse_position.y * 2.0f);
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

	// Update mouse position in RmlUi before processing button events
	rml_context->ProcessMouseMove((int)last_mouse_position.x, (int)last_mouse_position.y, 0);
	
	// Check hover state after updating position
	pointer_over_menu_icon = is_mouse_over_menu_icon();
	
	if (!pointer_over_menu_icon) {
		return false; // Not over menu icon, don't process
	}
	
	// Since GetElementAtPoint isn't working, find the icon directly based on mouse Y position
	// Container is at Y=110, each icon is 100px tall with 15px margin between them
	Rml::Element* container = menu_icons_document->GetElementById("menu_icons_container");
	if (!container) {
		return false;
	}
	
	float container_top = container->GetAbsoluteTop();
	float mouse_y_relative = last_mouse_position.y - container_top;
	
	Rml::Element* target_icon = nullptr;
	
	// Determine which icon was clicked based on Y position
	// Icon 1 (sound): 0-100px
	// Icon 2 (settings): 115-215px  
	// Icon 3 (exit): 230-330px
	if (mouse_y_relative >= 0 && mouse_y_relative < 100) {
		target_icon = menu_icons_document->GetElementById("sound_icon");
	} else if (mouse_y_relative >= 115 && mouse_y_relative < 215) {
		target_icon = menu_icons_document->GetElementById("settings_icon");
	} else if (mouse_y_relative >= 230 && mouse_y_relative < 330) {
		target_icon = menu_icons_document->GetElementById("exit_icon");
	}
	
	if (action == GLFW_PRESS) {
		rml_context->ProcessMouseButtonDown(button, 0);
		pointer_down_on_icon = true;
		
		// Dispatch click event directly to the target icon
		if (target_icon) {
			target_icon->DispatchEvent("click", Rml::Dictionary());
		}
		
		return true;
	} else if (action == GLFW_RELEASE) {
		rml_context->ProcessMouseButtonUp(button, 0);
		pointer_down_on_icon = false;
		return true;
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
	if (!element) {
		return false;
	}

	// Check if hover element belongs to our document
	Rml::ElementDocument* hover_doc = element->GetOwnerDocument();
	if (hover_doc != menu_icons_document) {
		// Try to find menu icon elements at the mouse position manually
		Rml::Element* container = menu_icons_document->GetElementById("menu_icons_container");
		if (container) {
			Rml::Vector2f mouse_pos = last_mouse_position;
			float container_left = container->GetAbsoluteLeft();
			float container_top = container->GetAbsoluteTop();
			float container_width = container->GetOffsetWidth();
			float container_height = container->GetOffsetHeight();
			
			// Check if mouse is within container bounds
			if (mouse_pos.x >= container_left && mouse_pos.x <= container_left + container_width &&
			    mouse_pos.y >= container_top && mouse_pos.y <= container_top + container_height) {
				return true;
			}
		}
		return false;
	}

	int depth = 0;
	while (element && depth < 10) {
		Rml::String id = element->GetId();
		
		if (id == "sound_icon" || id == "sound_icon_img" || id == "settings_icon" || id == "exit_icon" || id == "menu_icons_container") {
			return true;
		}

		Rml::Element* parent = element->GetParentNode();
		if (parent == element || parent == nullptr) {
			break;
		}
		element = parent;
		depth++;
	}

	return false;
}
#endif

