#include "objectives_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

ObjectivesSystem::ObjectivesSystem()
{
}

ObjectivesSystem::~ObjectivesSystem()
{
#ifdef HAVE_RMLUI
	if (objectives_document) {
		objectives_document->Close();
	}
#endif
}

#ifdef HAVE_RMLUI
bool ObjectivesSystem::init(Rml::Context* context)
#else
bool ObjectivesSystem::init(void* context)
#endif
{
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null for objectives" << std::endl;
		return false;
	}

	objectives_document = rml_context->LoadDocument("ui/objectives.rml");
	
	if (!objectives_document) {
		std::cerr << "ERROR: Failed to load objectives document" << std::endl;
		return false;
	}

	objectives_document->Show();
	set_visible(false);
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - Objectives disabled" << std::endl;
	return false;
#endif
}

void ObjectivesSystem::update(float elapsed_ms)
{
	(void)elapsed_ms; // Suppress unused warning
	// Objectives are updated via set_objective()
}

void ObjectivesSystem::render()
{
}

void ObjectivesSystem::set_visible(bool visible)
{
#ifdef HAVE_RMLUI
	if (!objectives_document) {
		return;
	}

	Rml::Element* container = objectives_document->GetElementById("objectives_container");
	if (!container) {
		return;
	}

	container->SetClass("hud-visible", visible);
	container->SetClass("hud-hidden", !visible);
#else
	(void)visible;
#endif
}

void ObjectivesSystem::play_intro_animation()
{
	set_visible(true);
}

void ObjectivesSystem::set_objective(int objective_num, bool completed, const std::string& text)
{
#ifdef HAVE_RMLUI
	if (!objectives_document) {
		return;
	}
	
	// Build element IDs
	char status_id[32];
	char text_id[32];
	snprintf(status_id, sizeof(status_id), "obj%d_status", objective_num);
	snprintf(text_id, sizeof(text_id), "obj%d_text", objective_num);
	
	// Update status icon
	Rml::Element* status_element = objectives_document->GetElementById(status_id);
	if (status_element) {
		// Build checkmark and cross element IDs
		char checkmark_id[32];
		char cross_id[32];
		snprintf(checkmark_id, sizeof(checkmark_id), "obj%d_checkmark", objective_num);
		snprintf(cross_id, sizeof(cross_id), "obj%d_cross", objective_num);
		
		Rml::Element* checkmark_img = objectives_document->GetElementById(checkmark_id);
		Rml::Element* cross_text = objectives_document->GetElementById(cross_id);
		
		if (completed) {
			// Show checkmark, hide cross
			if (checkmark_img) {
				checkmark_img->SetProperty("display", "inline-block");
			}
			if (cross_text) {
				cross_text->SetProperty("display", "none");
			}
			status_element->SetClass("cross", false);
		} else {
			// Show cross, hide checkmark
			if (checkmark_img) {
				checkmark_img->SetProperty("display", "none");
			}
			if (cross_text) {
				cross_text->SetProperty("display", "inline-block");
			}
			status_element->SetClass("cross", true);
		}
	}
	
	// Update text
	Rml::Element* text_element = objectives_document->GetElementById(text_id);
	if (text_element) {
		text_element->SetInnerRML(text.c_str());
	}
#else
	(void)objective_num;
	(void)completed;
	(void)text;
#endif
}

void ObjectivesSystem::set_circle_level(int circle_count)
{
#ifdef HAVE_RMLUI
	if (!objectives_document) {
		return;
	}
	
	// Update circle level title (circle_count is 0-indexed, so add 1 for display)
	Rml::Element* title_element = objectives_document->GetElementById("objectives_title");
	if (title_element) {
		char title_text[64];
		int display_level = circle_count + 1; // Convert 0-indexed to 1-indexed for display
		snprintf(title_text, sizeof(title_text), "Circle %d", display_level);
		title_element->SetInnerRML(title_text);
	}
#else
	(void)circle_count;
#endif
}

