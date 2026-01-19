#include "currency_system.hpp"
#include "tiny_ecs_registry.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

CurrencySystem::CurrencySystem()
{
}

CurrencySystem::~CurrencySystem()
{
#ifdef HAVE_RMLUI
	// Note: Don't call currency_document->Close() here to avoid crash during RmlUI shutdown
	// RmlUI will clean up documents when Rml::Shutdown() is called in main.cpp
#endif
}

#ifdef HAVE_RMLUI
bool CurrencySystem::init(Rml::Context* context)
#else
bool CurrencySystem::init(void* context)
#endif
{
#ifdef HAVE_RMLUI
	rml_context = context;
	
	if (!rml_context) {
		std::cerr << "ERROR: RmlUi context is null for currency" << std::endl;
		return false;
	}

	// Load the shared HUD document (contains both level and currency)
	currency_document = rml_context->LoadDocument("ui/hud_top_right.rml");
	
	if (!currency_document) {
		currency_document = rml_context->LoadDocument("../ui/hud_top_right.rml");
	}
	
	if (!currency_document) {
		std::cerr << "ERROR: Failed to load HUD document" << std::endl;
		return false;
	}

	currency_document->Show();
	set_visible(false);
	
	// Initialize with 0 currency
	update_currency(0);
	
	return true;
#else
	(void)context; // Suppress unused warning
	std::cerr << "ERROR: RmlUi not available - Currency disabled" << std::endl;
	return false;
#endif
}

void CurrencySystem::update_currency(int amount)
{
#ifdef HAVE_RMLUI
	if (!currency_document) {
		return;
	}
	
	current_currency = amount;
	
	Rml::Element* currency_amount = currency_document->GetElementById("currency_amount");
	if (currency_amount) {
		char amount_str[32];
		snprintf(amount_str, sizeof(amount_str), "%d", current_currency);
		currency_amount->SetInnerRML(amount_str);
	}
#else
	(void)amount;
#endif
}

void CurrencySystem::render()
{
}

void CurrencySystem::set_visible(bool visible)
{
#ifdef HAVE_RMLUI
	if (!currency_document) {
		return;
	}

	Rml::Element* container = currency_document->GetElementById("hud_top_right_container");
	if (!container) {
		return;
	}

	container->SetClass("hud-visible", visible);
	container->SetClass("hud-hidden", !visible);
#else
	(void)visible;
#endif
}

void CurrencySystem::play_intro_animation()
{
	set_visible(true);
}

