#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class CurrencySystem
{
public:
	CurrencySystem();
	~CurrencySystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context);
#else
	bool init(void* context);
#endif
	
	void update_currency(int amount);

	// Render the currency UI
	void render();

	void play_intro_animation();
	void set_visible(bool visible);
	
	// Get the document (for shared access by other systems)
#ifdef HAVE_RMLUI
	Rml::ElementDocument* get_document() const { return currency_document; }
#endif

private:
#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* currency_document = nullptr;
#endif
	
	int current_currency = 0;
};

