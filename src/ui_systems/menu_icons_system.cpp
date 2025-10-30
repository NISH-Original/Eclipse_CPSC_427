#include "menu_icons_system.hpp"
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
bool MenuIconsSystem::init(Rml::Context* context)
#else
bool MenuIconsSystem::init(void* context)
#endif
{
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

	menu_icons_document->Show();
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - Menu Icons disabled" << std::endl;
	return false;
#endif
}

void MenuIconsSystem::render()
{
}

