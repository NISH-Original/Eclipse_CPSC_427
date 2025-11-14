#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "render_system.hpp"
#include "inventory_system.hpp"
#include "stats_system.hpp"
#include "objectives_system.hpp"
#include "minimap_system.hpp"
#include "currency_system.hpp"
#include "audio_system.hpp"
#include "tutorial_system.hpp"
#include "noise_gen.hpp"
#include "level_manager.hpp"

// Forward declaration
class AISystem;

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	WorldSystem();

	// Creates a window
	GLFWwindow* create_window();

	// starts the game
	void init(RenderSystem* renderer, InventorySystem* inventory, StatsSystem* stats, ObjectivesSystem* objectives, MinimapSystem* minimap, CurrencySystem* currency, TutorialSystem* tutorial, AISystem* ai, AudioSystem* audio);

	// Releases all associated resources
	~WorldSystem();

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Check for collisions
	void handle_collisions();

	// Should the game be over ?
	bool is_over()const;
	
	// Exit bonfire mode (called when inventory is closed)
	void exit_bonfire_mode();
private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 pos);
	void on_mouse_click(int button, int action, int mods);
	void fire_weapon();

	// restart level
	void restart_game();
	void spawn_enemies(float elapsed_seconds);
	
	// get weapon texture based on equipped weapon
	TEXTURE_ASSET_ID get_weapon_texture(TEXTURE_ASSET_ID base_texture) const;
	// get hurt texture based on equipped weapon
	TEXTURE_ASSET_ID get_hurt_texture() const;

	// OpenGL window handle
	GLFWwindow* window;

	// Score, displayed in the window title
	unsigned int points;

	// Game state
	RenderSystem* renderer;
	InventorySystem* inventory_system;
	StatsSystem* stats_system;
	ObjectivesSystem* objectives_system;
	MinimapSystem* minimap_system;
	CurrencySystem* currency_system;
	AudioSystem* audio_system;
	TutorialSystem* tutorial_system;
	float current_speed;
	Entity player_salmon;
	Entity player_feet;
	Entity player_dash;
	Entity flashlight;
	Entity background;

	// Player input tracking
	// TODO: refactor these into a bitfield (had trouble with that before)
	bool left_pressed;
	bool right_pressed;
	bool up_pressed;
	bool down_pressed;
	bool prioritize_right;
	bool prioritize_down;
	vec2 mouse_pos;
	bool left_mouse_pressed = false;
	float fire_rate_cooldown = 0.0f;
	bool rifle_sound_playing = false;
	float rifle_sound_start_time = 0.0f;
	float current_time_seconds = 0.0f; // current time in seconds
	float rifle_sound_min_duration = 0.13f;
	
	// Dash system
	bool is_dashing;
	float dash_timer;
	float dash_cooldown_timer;
	vec2 dash_direction; // lock direction during dash
	const float dash_duration = 0.2f;
	const float dash_cooldown = 1.0f;
	const float dash_multiplier = 3.0f; // velocity multiplier
	const float dash_sprite_offset = 50.0f; // offset behind player along dash direction
	const float dash_sprite_side_offset = -5.0f; // offset to the side of player
	
	// weapon knockback system
	bool is_knockback;
	float knockback_timer;
	vec2 knockback_direction; // opposite of shooting
	const float knockback_duration = 0.15f;
	const float knockback_multiplier = 4.0f; // velocity multiplier

	// hurt knockback system (from enemy collisions)
	bool is_hurt_knockback;
	float hurt_knockback_timer;
	vec2 hurt_knockback_direction; // opposite of collision direction
	const float hurt_knockback_duration = 0.15f;
	const float hurt_knockback_multiplier = 4.0f; // velocity multiplier
	TEXTURE_ASSET_ID animation_before_hurt; // store animation to resume after hurt

	// spawn system
	float spawn_timer = 0.0f;
	float wave_timer = 0.0f;
	int wave_count = 0;


	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	// Noise generator
	PerlinNoiseGenerator map_perlin;
	PerlinNoiseGenerator decorator_perlin;
	
	// Objectives tracking
	float survival_time_ms = 0.f;
	int kill_count = 0;
	
	bool player_was_in_radius = true;
	bool bonfire_spawned = false; // Track if bonfire has been spawned after objectives complete
	
	// Camera lerp to bonfire
	bool is_camera_lerping_to_bonfire = false;
	bool is_camera_locked_on_bonfire = false;
	vec2 camera_lerp_start = { 0.f, 0.f };
	vec2 camera_lerp_target = { 0.f, 0.f };
	float camera_lerp_time = 0.f;
	const float CAMERA_LERP_DURATION = 1000.0f;
	
	// Player angle lerp to bonfire
	bool is_player_angle_lerping = false;
	float player_angle_lerp_start = 0.f;
	float player_angle_lerp_target = 0.f;
	float player_angle_lerp_time = 0.f;
	const float PLAYER_ANGLE_LERP_DURATION = 500.0f;
	
	// Flag to open inventory after bonfire interaction lerping completes
	bool should_open_inventory_after_lerp = false;
	
	// Arrow entity reference
	Entity arrow_entity = Entity();
	bool arrow_exists = false;
	
	// Bonfire entity reference (to prevent it from being removed during chunk cleanup)
	Entity bonfire_entity = Entity();
	bool bonfire_exists = false;
	
	// Level manager for tracking circles and spawn radius
	LevelManager level_manager;
};
