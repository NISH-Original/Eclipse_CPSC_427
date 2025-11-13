#pragma once

#include "common.hpp"

class AudioSystem;

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

class MenuIconsSystem
#ifdef HAVE_RMLUI
	: public Rml::EventListener
#endif
{
public:
	MenuIconsSystem();
	~MenuIconsSystem();
	
#ifdef HAVE_RMLUI
	bool init(Rml::Context* context, AudioSystem* audio);
	void ProcessEvent(Rml::Event& event) override;
#else
	bool init(void* context, AudioSystem* audio);
#endif
	void on_mouse_move(vec2 mouse_position);
	bool on_mouse_button(int button, int action, int mods);

	// Render the menu icons UI
	void render();

	void play_intro_animation();
	void set_visible(bool visible);

private:
#ifdef HAVE_RMLUI
	void update_sound_icon();
	bool is_mouse_over_menu_icon() const;
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* menu_icons_document = nullptr;
	Rml::Vector2f last_mouse_position = {0.f, 0.f};
	bool pointer_over_menu_icon = false;
	bool pointer_down_on_icon = false;
#endif
	AudioSystem* audio_system = nullptr;
};
