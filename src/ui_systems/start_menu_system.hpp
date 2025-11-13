#pragma once

#include "common.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

#include <functional>

class AudioSystem;

class StartMenuSystem
#ifdef HAVE_RMLUI
	: public Rml::EventListener
#endif
{
public:
	StartMenuSystem();
	~StartMenuSystem();

#ifdef HAVE_RMLUI
	bool init(Rml::Context* context, AudioSystem* audio);
#else
	bool init(void* context, AudioSystem* audio);
#endif

	void show();
	void hide_immediately();
	void begin_exit_sequence();

	bool is_visible() const { return menu_visible; }
	bool is_exiting() const { return menu_exiting; }
	bool is_supported() const { return menu_supported; }

	void on_mouse_move(vec2 mouse_position);
	bool on_mouse_button(int button, int action, int mods);
	bool on_key(int key, int action, int mods);

	void set_start_game_callback(std::function<void()> callback) { on_start_game = std::move(callback); }
	void set_continue_callback(std::function<void()> callback) { on_continue_game = std::move(callback); }
	void set_exit_callback(std::function<void()> callback) { on_exit = std::move(callback); }
	void set_open_settings_callback(std::function<void()> callback) { on_open_settings = std::move(callback); }
	void set_open_tutorials_callback(std::function<void()> callback) { on_open_tutorials = std::move(callback); }
	void set_menu_hidden_callback(std::function<void()> callback) { on_menu_hidden = std::move(callback); }

private:
#ifdef HAVE_RMLUI
	void ProcessEvent(Rml::Event& event) override;
	void attach_listeners();
	void update_pointer_state(vec2 mouse_position);

	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* start_menu_document = nullptr;
	Rml::Element* root_container = nullptr;
#endif
	AudioSystem* audio_system = nullptr;

	bool pointer_over_button = false;
	bool pointer_pressed_on_button = false;
	bool menu_visible = false;
	bool menu_exiting = false;
	bool menu_supported = false;

	std::function<void()> on_start_game;
	std::function<void()> on_continue_game;
	std::function<void()> on_exit;
	std::function<void()> on_open_settings;
	std::function<void()> on_open_tutorials;
	std::function<void()> on_menu_hidden;
};

