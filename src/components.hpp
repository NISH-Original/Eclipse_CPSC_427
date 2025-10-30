#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

// Player component
struct Player
{
	int health = 100;
	int max_health = 100;
	int armour = 0;
	int max_armour = 100;
	int currency = 1000;
	// weapon ammo state (for pistol)
	int magazine_size = 10;
	int ammo_in_mag = 10;
};

// Weapon types
enum class WeaponType {
	LASER_PISTOL_GREEN,
	LASER_PISTOL_RED,
	PLASMA_SHOTGUN_HEAVY,
	PLASMA_SHOTGUN_UNSTABLE,
	SNIPER_RIFLE,
	WEAPON_COUNT
};

// Armor/Suit types
enum class ArmorType {
	BASIC_SUIT,
	ADVANCED_SUIT,
	HEAVY_SUIT,
	ARMOR_COUNT
};

// Item rarity/quality
enum class ItemRarity {
	COMMON,
	UNCOMMON,
	RARE,
	EPIC,
	LEGENDARY
};

// Weapon component
struct Weapon {
	WeaponType type;
	std::string name;
	std::string description;
	int damage = 10;
	int price = 0;
	bool owned = false;
	bool equipped = false;
	ItemRarity rarity = ItemRarity::COMMON;
};

// Armor component
struct Armor {
	ArmorType type;
	std::string name;
	std::string description;
	int defense = 5;
	int price = 0;
	bool owned = false;
	bool equipped = false;
	ItemRarity rarity = ItemRarity::COMMON;
};

// Player Inventory
struct Inventory {
	std::vector<Entity> weapons;
	std::vector<Entity> armors;
	Entity equipped_weapon;
	Entity equipped_armor;
	bool is_open = false;
};

// Obstacle component
struct Obstacle
{

};

struct Enemy {
	bool is_dead = false;
	int damage = 10;
};

// Cooldown timer for taking damage (prevents continuous damage)
struct DamageCooldown {
	float cooldown_ms = 0.f;
	float max_cooldown_ms = 1000.f; // 1 second cooldown between hits
};


enum class TEXTURE_ASSET_ID;

struct Sprite {
	int total_frame;
	int curr_frame = 0;
	float step_seconds_acc = 0.0f;
	bool should_flip = false;
    float animation_speed = 10.0f;
	
	// animation state tracking for player
	TEXTURE_ASSET_ID current_animation;
	int idle_frames = 20;
	int move_frames = 20;
	int shoot_frames = 3;
	
	bool is_shooting = false;
	float shoot_timer = 0.0f;
	float shoot_duration = 0.3f; // duration of shooting animation (for pistol)
	TEXTURE_ASSET_ID previous_animation; // store what animation to return to

	// reload animation state
	bool is_reloading = false;
	int reload_frames = 15;
	float reload_timer = 0.0f;
	float reload_duration = 1.5f; // seconds
};

struct Bullet {

};

struct Feet {
	Entity parent_player; // the player this feet belongs to
};

// Treats screen boundaries as impassible walls
struct ConstrainedToScreen
{

};

// Component to mark entities that cast shadows
struct Occluder {
	bool casts_shadows = true;
};

// All data relevant to the shape and motion of entities
struct Motion {
	vec2 position = { 0, 0 };
	float angle = 0;
	vec2 velocity = { 0, 0 };
	vec2 scale = { 10, 10 };
};

// Stucture to store collision information
struct Collision
{
	// Note, the first object is stored in the ECS container.entities
	Entity other; // the second object involved in the collision
	Collision(Entity& other) { this->other = other; };
};

// Data structure for toggling debug mode
struct Debug {
	bool in_debug_mode = 0;
	bool in_freeze_mode = 0;
	bool show_occlusion_mask = 0;
};
extern Debug debugging;

struct Light {
	float cone_angle = 1.0f;
	float brightness = 1.0f;
	float falloff = 1.0f;
	float range = 200.0f;
	vec3 light_color = { 1.0f, 1.0f, 1.0f };
	bool is_enabled = false;
	float inner_cone_angle = 0.0f;
	Entity follow_target;
	vec2 offset = { 0.0f, 0.0f };
	bool use_target_angle = true;
};

// Sets the brightness of the screen
struct ScreenState
{
	float darken_screen_factor = -1;
};

// A struct to refer to debugging graphics in the ECS
struct DebugComponent
{
	// Note, an empty struct has size 1
};

// A timer that will be associated to dying salmon
struct DeathTimer
{
	float counter_ms = 3000;
};

// Single Vertex Buffer element for non-textured meshes (coloured.vs.glsl & salmon.vs.glsl)
struct ColoredVertex
{
	vec3 position;
	vec3 color;
};

// Single Vertex Buffer element for textured sprites (textured.vs.glsl)
struct TexturedVertex
{
	vec3 position;
	vec2 texcoord;
};

// Mesh datastructure for storing vertex and index buffers
struct Mesh
{
	static bool loadFromOBJFile(std::string obj_path, std::vector<ColoredVertex>& out_vertices, std::vector<uint16_t>& out_vertex_indices, vec2& out_size);
	vec2 original_size = {1,1};
	std::vector<ColoredVertex> vertices;
	std::vector<uint16_t> vertex_indices;
};

/**
 * The following enumerators represent global identifiers refering to graphic
 * assets. For example TEXTURE_ASSET_ID are the identifiers of each texture
 * currently supported by the system.
 *
 * So, instead of referring to a game asset directly, the game logic just
 * uses these enumerators and the RenderRequest struct to inform the renderer
 * how to structure the next draw command.
 *
 * There are 2 reasons for this:
 *
 * First, game assets such as textures and meshes are large and should not be
 * copied around as this wastes memory and runtime. Thus separating the data
 * from its representation makes the system faster.
 *
 * Second, it is good practice to decouple the game logic from the render logic.
 * Imagine, for example, changing from OpenGL to Vulkan, if the game logic
 * depends on OpenGL semantics it will be much harder to do the switch than if
 * the renderer encapsulates all asset data and the game logic is agnostic to it.
 *
 * The final value in each enumeration is both a way to keep track of how many
 * enums there are, and as a default value to represent uninitialized fields.
 */

enum class TEXTURE_ASSET_ID {
    SLIME = 0,
    TREE = SLIME + 1,
    PLAYER_IDLE = TREE + 1,
    PLAYER_MOVE = PLAYER_IDLE + 1,
    PLAYER_SHOOT = PLAYER_MOVE + 1,
	PLAYER_RELOAD = PLAYER_SHOOT + 1,
	FEET_WALK = PLAYER_RELOAD + 1,
	TEXTURE_COUNT = FEET_WALK + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	TEXTURED = COLOURED + 1,
	SALMON = TEXTURED + 1,
	WATER = SALMON + 1,
	LIGHT = WATER + 1,
	EFFECT_COUNT = LIGHT + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
	SPRITE = 0,
	BULLET_CIRCLE = SPRITE + 1,
	ENEMY_TRIANGLE = BULLET_CIRCLE + 1,
	SCREEN_TRIANGLE = ENEMY_TRIANGLE + 1,
	BACKGROUND_QUAD = SCREEN_TRIANGLE + 1,
	HEALTH_BAR = BACKGROUND_QUAD + 1,
	GEOMETRY_COUNT = HEALTH_BAR + 1,
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};
