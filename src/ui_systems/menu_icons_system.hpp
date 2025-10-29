#pragma once

#include "common.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class MenuIconsSystem
{
public:
	MenuIconsSystem();
	~MenuIconsSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif

	// Render the menu icons UI
	void render();

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* menu_icons_document = nullptr;
#endif
};

