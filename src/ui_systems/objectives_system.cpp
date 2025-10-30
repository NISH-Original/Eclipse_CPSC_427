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
		if (completed) {
			status_element->SetInnerRML("&#10003;"); // Checkmark
			status_element->SetClass("cross", false);
		} else {
			status_element->SetInnerRML("&#10007;"); // X mark
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

