#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

#include <string>

class ObjectivesSystem
{
public:
	ObjectivesSystem();
	~ObjectivesSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif

	// Update objectives data
	void update(float elapsed_ms);

	// Render the objectives UI
	void render();

	// Set objective status and text
	void set_objective(int objective_num, bool completed, const std::string& text);
	
	// Update circle level display
	void set_circle_level(int circle_count);

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* objectives_document = nullptr;
#endif
};

