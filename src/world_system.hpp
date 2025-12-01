#pragma once

// internal
#include "common.hpp"

// stlib
#include <vector>
#include <random>
#include <json.hpp>

using json = nlohmann::json;

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "render_system.hpp"
#include "inventory_system.hpp"
#include "stats_system.hpp"
#include "objectives_system.hpp"
#include "currency_system.hpp"
#include "audio_system.hpp"
#include "tutorial_system.hpp"
#include "menu_icons_system.hpp"
#include "health_system.hpp"
#include "noise_gen.hpp"
#include "level_manager.hpp"

// Forward declaration
class AISystem;
class StartMenuSystem;
class SaveSystem;
class DeathScreenSystem;

// Container for all our entities and game logic. Individual rendering / update is
// deferred to the relative update() methods
class WorldSystem
{
public:
	WorldSystem();

	// Creates a window
	GLFWwindow* create_window();

	// starts the game
	void init(RenderSystem* renderer, InventorySystem* inventory, StatsSystem* stats, ObjectivesSystem* objectives, CurrencySystem* currency, MenuIconsSystem* menu_icons, TutorialSystem* tutorial, StartMenuSystem* start_menu, AISystem* ai, AudioSystem* audio, SaveSystem* save_system, DeathScreenSystem* death_screen);

	// Releases all associated resources
	~WorldSystem();

	// Steps the game ahead by ms milliseconds
	bool step(float elapsed_ms);

	// Update state while gameplay simulation is paused (e.g., during start menu)
	void update_paused(float elapsed_ms);

	// Check for collisions
	void handle_collisions();
	
	// sync feet and dash position to player
	void sync_feet_to_player();
	
	// Helper function to apply damage to an enemy (used by both bullets and flashlight)
	void apply_enemy_damage(Entity enemy_entity, int damage, vec2 damage_direction, bool create_blood = true);
	
	// Helper function to apply damage to the player (unified on-hit logic)
	// Returns true if player died
	bool on_player_hit(int raw_damage, vec2 damage_source_position);
	
	// Helper function to handle player death
	void handle_player_death();
	
	// Helper function to detonate an explosive bullet
	void detonate_bullet(const Bullet& bullet, const Motion& bullet_motion);

	// Should the game be over ?
	bool is_over()const;
	
	// Exit bonfire mode (called when inventory is closed)
	void exit_bonfire_mode();

	void finalize_start_menu_transition();

	bool is_start_menu_active() const { return start_menu_active; }
	bool is_level_transition_active() const { return is_level_transitioning; }
	void request_start_game();
	void request_return_to_menu();

	void update_crosshair_cursor();

	json serialize() const;
	void deserialize(const json& data);

	// hurt knockback system (from enemy collisions)
	bool is_hurt_knockback;
	float hurt_knockback_timer;
	vec2 hurt_knockback_direction; // opposite of collision direction
	const float hurt_knockback_duration = 0.15f;
	const float hurt_knockback_multiplier = 4.0f; // velocity multiplier
	TEXTURE_ASSET_ID animation_before_hurt; // store animation to resume after hurt
	AudioSystem* audio_system;

private:
	// Input callback functions
	void on_key(int key, int, int action, int mod);
	void on_mouse_move(vec2 pos);
	void on_mouse_click(int button, int action, int mods);
	void fire_weapon();
	void start_reload(); // Helper function to start reload animation

	// restart level
	void restart_game();
	void spawn_enemies(float elapsed_seconds);
	
	// get weapon texture based on equipped weapon
	TEXTURE_ASSET_ID get_weapon_texture(TEXTURE_ASSET_ID base_texture) const;
	// get hurt texture based on equipped weapon
	TEXTURE_ASSET_ID get_hurt_texture() const;

	void play_hud_intro();

	// OpenGL window handle
	GLFWwindow* window;
	
	// Custom cursors for different weapons
	GLFWcursor* pistol_crosshair_cursor = nullptr;
	GLFWcursor* shotgun_crosshair_cursor = nullptr;
	GLFWcursor* rifle_crosshair_cursor = nullptr;
	GLFWcursor* explosive_crosshair_cursor = nullptr;

	// Score, displayed in the window title
	unsigned int points;

	// Game state
	RenderSystem* renderer;
	InventorySystem* inventory_system;
	StatsSystem* stats_system;
	ObjectivesSystem* objectives_system;
	CurrencySystem* currency_system;
	MenuIconsSystem* menu_icons_system = nullptr;
	TutorialSystem* tutorial_system;
	StartMenuSystem* start_menu_system = nullptr;
	SaveSystem* save_system = nullptr;
	DeathScreenSystem* death_screen_system = nullptr;
	HealthSystem health_system;
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
	bool heartbeat_playing = false; // Track if heartbeat sound is playing
	
	// Dash system
	bool is_dashing;
	float dash_timer;
	float dash_cooldown_timer;
	vec2 dash_direction; // lock direction during dash
	float dash_trail_accum = 0.f; // accumulator for particle trail spawning
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
	const float knockback_multiplier = 2.0f; // velocity multiplier

	// spawn system
	float spawn_timer = 0.0f;
	float wave_timer = 0.0f;
	int wave_count = 0;
	
	// Level tracking - separate from waves
	int current_level = 1;
	bool xylarite_crab_spawned_this_level = false; // Track if XylariteCrab has been spawned this level


	// C++ random number generator
	std::default_random_engine rng;
	std::uniform_real_distribution<float> uniform_dist; // number between 0..1

	// World generation data
	PerlinNoiseGenerator map_perlin;
	PerlinNoiseGenerator decorator_perlin;
	unsigned int map_seed = 0;
	unsigned int decorator_seed = 0;

	// Objectives tracking
	float survival_time_ms = 0.f;
	int kill_count = 0;
	
	bool player_was_in_radius = true;
	bool bonfire_spawned = false; // Track if bonfire has been spawned after objectives complete
	
	struct BonfireInventoryState {
		bool has_state = false;
		float saved_survival_time_ms = 0.f;
		int saved_kill_count = 0;
		bool saved_bonfire_spawned = false;
		Entity bonfire_entity = Entity();
		TEXTURE_ASSET_ID saved_bonfire_texture = TEXTURE_ASSET_ID::BONFIRE;
	} bonfire_inventory_state;
	
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
	
	// Track bonfire positions for each circle (circle index -> bonfire position)
	// Circle 0 uses the initial spawn position, subsequent circles use bonfire positions
	std::vector<vec2> circle_bonfire_positions;
	vec2 initial_spawn_position; // Store the initial spawn position for circle 0
	
	// Level manager for tracking circles and spawn radius
	LevelManager level_manager;

	// Start menu & intro state
	bool start_menu_active = false;
	bool start_menu_transitioning = false;
	bool gameplay_started = false;
	bool game_session_active = false;
	bool start_camera_lerping = false;
	vec2 start_menu_camera_focus = {0.f, 0.f};
	vec2 start_camera_lerp_start = {0.f, 0.f};
	vec2 start_camera_lerp_target = {0.f, 0.f};
	float start_camera_lerp_time = 0.f;
	const float START_CAMERA_LERP_DURATION = 900.0f;

	bool hud_intro_played = false;
	bool should_start_tutorial_on_menu_hide = false;
	float menu_hide_tutorial_fallback_timer = 0.0f;
	const float MENU_HIDE_TUTORIAL_FALLBACK_DURATION = 1000.0f; // 1 second fallback

	// Death screen state
	bool death_screen_shown = false;
	float death_screen_timer = 0.0f;
	const float DEATH_SCREEN_DURATION = 3000.0f; // 3 seconds before restarting

	// Bonfire instructions UI
#ifdef HAVE_RMLUI
	Rml::ElementDocument* bonfire_instructions_document = nullptr;
	bool is_near_bonfire = false;
	Entity current_bonfire_entity = Entity(); // Track which bonfire we're near
	Rml::ElementDocument* level_transition_document = nullptr;
	bool is_level_transitioning = false;
	float level_transition_timer = 0.0f;
	const float LEVEL_TRANSITION_DURATION = 3.0f; // 3 seconds countdown
#endif

	// Helper functions for bonfire instructions
	void update_bonfire_instructions();
	void update_bonfire_instructions_position();
	void show_bonfire_instructions();
	void hide_bonfire_instructions();
	void handle_next_level();
	void update_level_display();
	void update_level_transition_countdown();
	void complete_level_transition();
	void on_inventory_closed(bool cancelled);
	void save_pre_inventory_state(Entity bonfire_entity, TEXTURE_ASSET_ID previous_texture);
	void restore_pre_inventory_state();
	void clear_pre_inventory_state();
};