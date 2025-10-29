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
	if (currency_document) {
		currency_document->Close();
	}
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

	// Load the currency document
	currency_document = rml_context->LoadDocument("ui/currency.rml");
	
	if (!currency_document) {
		std::cerr << "ERROR: Failed to load currency document" << std::endl;
		return false;
	}

	// Show the currency display (always visible)
	currency_document->Show();
	
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

