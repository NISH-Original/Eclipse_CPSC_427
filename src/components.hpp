#pragma once
#include "common.hpp"
#include <vector>
#include <unordered_map>
#include "../ext/stb_image/stb_image.h"

// Player component
struct Player
{
	float health = 100.0f;
	float max_health = 100.0f;
	int armour = 0;
	int max_armour = 100;
	int currency = 0;
	// weapon ammo state (for pistol)
	int magazine_size = 10;
	int ammo_in_mag = 10;
	float speed = 200.0f;
	// visual rendering offset (does not affect collision)
	vec2 render_offset = {0.0f, -6.0f};
};

struct PlayerUpgrades
{
	int movement_speed_level = 0;
	int max_health_level = 0;
	int armour_level = 0;
	int light_radius_level = 0;
	int dash_cooldown_level = 0;
	int health_regen_level = 0;
	int crit_chance_level = 0;
	int life_steal_level = 0;
	int flashlight_width_level = 0;
	int flashlight_damage_level = 0;
	int flashlight_slow_level = 0;
	int xylarite_multiplier_level = 0;

	static constexpr int MAX_UPGRADE_LEVEL = 5;

	static constexpr int MOVEMENT_SPEED_COST = 100;
	static constexpr int MAX_HEALTH_COST = 150;
	static constexpr int ARMOUR_COST = 150;
	static constexpr int LIGHT_RADIUS_COST = 75;
	static constexpr int DASH_COOLDOWN_COST = 125;
	static constexpr int HEALTH_REGEN_COST = 200;
	static constexpr int CRIT_CHANCE_COST = 175;
	static constexpr int LIFE_STEAL_COST = 225;
	static constexpr int FLASHLIGHT_WIDTH_COST = 125;
	static constexpr int FLASHLIGHT_DAMAGE_COST = 200;
	static constexpr int FLASHLIGHT_SLOW_COST = 150;
	static constexpr int XYLARITE_MULTIPLIER_COST = 250;

	static constexpr float MOVEMENT_SPEED_PER_LEVEL = 20.0f;
	static constexpr int HEALTH_PER_LEVEL = 20;
	static constexpr int ARMOUR_PER_LEVEL = 5;
	static constexpr float LIGHT_RADIUS_PER_LEVEL = 50.0f;
	static constexpr float DASH_COOLDOWN_REDUCTION_PER_LEVEL = 0.15f;
	static constexpr float HEALTH_REGEN_PER_LEVEL = 1.0f;
	static constexpr float CRIT_CHANCE_PER_LEVEL = 0.05f;
	static constexpr float LIFE_STEAL_PER_LEVEL = 0.03f;
	static constexpr float FLASHLIGHT_WIDTH_PER_LEVEL = 0.1f;
	static constexpr float FLASHLIGHT_DAMAGE_PER_LEVEL = 5.0f;
	static constexpr float FLASHLIGHT_SLOW_PER_LEVEL = 10.0f;
	static constexpr float XYLARITE_MULTIPLIER_PER_LEVEL = 0.1f;
};

struct WeaponUpgrades
{
	int fire_rate_level = 0;
	int damage_level = 0;
	int ammo_capacity_level = 0;

	static constexpr int MAX_UPGRADE_LEVEL = 5;
	static constexpr int FIRE_RATE_COST = 80;
	static constexpr int DAMAGE_COST = 100;
	static constexpr int AMMO_CAPACITY_COST = 60;
	static constexpr float FIRE_RATE_MULTIPLIER_PER_LEVEL = 1.15f;
	static constexpr int DAMAGE_PER_LEVEL = 5;
	static constexpr int AMMO_PER_LEVEL = 3;
};

// Weapon types
enum class WeaponType {
	LASER_PISTOL_GREEN,
	LASER_PISTOL_RED,
	PLASMA_SHOTGUN_HEAVY,
	ASSAULT_RIFLE,
	SNIPER_RIFLE,
	WEAPON_COUNT
};

// armour/Suit types
enum class armourType {
	BASIC_SUIT,
	ADVANCED_SUIT,
	HEAVY_SUIT,
	armour_COUNT
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
	float fire_rate_rpm = 0.0f; // rounds per minute (0 = semi-automatic)
};

// armour component
struct armour {
	armourType type;
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
	std::vector<Entity> armours;
	Entity equipped_weapon;
	Entity equipped_armour;
	bool is_open = false;
};

// Obstacle component
struct Obstacle
{

};

struct Enemy {
	bool is_dead = false;
	bool is_hurt = false;
	bool death_handled = false;

	std::function<void(Entity, float)> death_animation = NULL; 
	std::function<void(Entity, float)> hurt_animation = NULL;
	float hurt_timer = 0.0f;
	float healthbar_visibility_timer = 0.0f;  // Timer for healthbar visibility after taking damage

	int damage = 10;
	int health = 100;
	int max_health = 100;

	int xylarite_drop = 1;
};

struct Steering {
	float target_angle;
	float rad_ms = 0.003;
	float vel;
};

struct AccumulatedForce {
	glm::vec2 v{ 0, 0 };
};

struct EnemyLunge {
	bool is_lunging = false;
	glm::vec2 lunge_direction = { 0, 0 };
	float lunge_timer = 0.f;
	float lunge_cooldown = 0.f;
	static constexpr float LUNGE_DURATION = 0.2f;
	static constexpr float LUNGE_COOLDOWN = 1.5f;
};

struct MovementAnimation {
	vec2 base_scale = { 100.f, 100.f };
	float squish_frequency = 8.0f;
	float squish_amount = 0.08f;
	float bounce_frequency = 6.0f;
	float bounce_amount = 3.0f;
	float animation_timer = 0.f;
};

// Cooldown timer for taking damage (prevents continuous damage)
struct DamageCooldown {
	float cooldown_ms = 0.f;
	float max_cooldown_ms = 1000.f; // 1 second cooldown between hits
};


enum class TEXTURE_ASSET_ID;

enum class StationaryEnemyState {
	EP_IDLE,
	EP_DETECT_PLAYER,
	EP_ATTACK_PLAYER,
	EP_COOLDOWN
};

enum class StationaryEnemyFacing {
	EP_FACING_LEFT,
	EP_FACING_RIGHT,
	EP_FACING_UP,
	EP_FACING_DOWN
};

struct StationaryEnemy {
	StationaryEnemyState state = StationaryEnemyState::EP_IDLE;
	StationaryEnemyFacing facing = StationaryEnemyFacing::EP_FACING_DOWN;
	float attack_cooldown;
	vec2 position = { 0, 0 };
};

struct Sprite {
	int total_row;
	int total_frame;
	int curr_row = 0;
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
	int damage = 25;
};

struct Deadly {
	// Components that belongs to enemies, but aren't actually enemies (Such as enemy bullet)
};

struct Feet {
	Entity parent_player; // the player this feet belongs to
	// visual rendering offset (does not affect collision)
	vec2 render_offset = {0.0f, -6.0f};
	bool transition_pending = false;
	TEXTURE_ASSET_ID transition_target;
	int transition_frame_primary = -1;
	int transition_frame_secondary = -1;
	int transition_start_frame = 0;
	int last_horizontal_sign = 0;
	TEXTURE_ASSET_ID locked_horizontal_texture = TEXTURE_ASSET_ID{};
	bool locked_texture_valid = false;
};

struct Arrow {
	// Arrow component to mark arrow entities (rendered at screen center, unaffected by lighting)
};

struct CollisionMesh {
    std::vector<vec2> local_points;
};

struct CollisionCircle {
    float radius = 0.f;
};

// Does not collide with other entities
struct NonCollider {

};

// bounding box for isoline obstacles
struct IsolineBoundingBox {
    vec2 center = { 0.f, 0.f };
    float half_width = 0.f;
    float half_height = 0.f; 
};

// Treats screen boundaries as impassible walls
struct ConstrainedToScreen {

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

struct Particle {
	vec3 position;
	vec3 velocity;
	vec4 color;
	float size;
	float lifetime;
	float age;
	bool alive;
};

struct Drop {
	bool is_magnetized = false;
	float magnet_timer = 0.f;
	float trail_accum = 0.f;
};

struct Trail {
  float life;
  float alpha;
	bool is_red = false;
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
	TRAIL = 0,
	FIRST_AID = TRAIL +1,
	XYLARITE = FIRST_AID + 1,
	XY_CRAB = XYLARITE + 1,
	SLIME_1 = XY_CRAB + 1,
	SLIME_2 = SLIME_1 + 1,
	SLIME_3 = SLIME_2 + 1,
	PLANT_IDLE_1 = SLIME_3 + 1,
	PLANT_ATTACK_1 = PLANT_IDLE_1 + 1,
	PLANT_HURT_1 = PLANT_ATTACK_1 + 1,
	PLANT_DEATH_1 = PLANT_HURT_1 + 1,
	PLANT_IDLE_2 = PLANT_DEATH_1 + 1,
	PLANT_ATTACK_2 = PLANT_IDLE_2 + 1,
	PLANT_HURT_2 = PLANT_ATTACK_2 + 1,
	PLANT_DEATH_2 = PLANT_HURT_2 + 1,
	PLANT_IDLE_3 = PLANT_DEATH_2 + 1,
	PLANT_ATTACK_3 = PLANT_IDLE_3 + 1,
	PLANT_HURT_3 = PLANT_ATTACK_3 + 1,
	PLANT_DEATH_3 = PLANT_HURT_3 + 1,
	TREE = PLANT_DEATH_3 + 1,
	PLAYER_IDLE = TREE + 1,
	PLAYER_MOVE = PLAYER_IDLE + 1,
	PLAYER_SHOOT = PLAYER_MOVE + 1,
	PLAYER_RELOAD = PLAYER_SHOOT + 1,
	SHOTGUN_IDLE = PLAYER_RELOAD + 1,
	SHOTGUN_MOVE = SHOTGUN_IDLE + 1,
	SHOTGUN_SHOOT = SHOTGUN_MOVE + 1,
	SHOTGUN_RELOAD = SHOTGUN_SHOOT + 1,
	RIFLE_IDLE = SHOTGUN_RELOAD + 1,
	RIFLE_MOVE = RIFLE_IDLE + 1,
	RIFLE_SHOOT = RIFLE_MOVE + 1,
	RIFLE_RELOAD = RIFLE_SHOOT + 1,
	PISTOL_HURT = RIFLE_RELOAD + 1,
	SHOTGUN_HURT = PISTOL_HURT + 1,
	RIFLE_HURT = SHOTGUN_HURT + 1,
	FEET_WALK = RIFLE_HURT + 1,
	FEET_LEFT = FEET_WALK + 1,
	FEET_RIGHT = FEET_LEFT + 1,
	DASH = FEET_RIGHT + 1,
	BONFIRE = DASH + 1,
	BONFIRE_OFF = BONFIRE + 1,
	ARROW = BONFIRE_OFF + 1,
	ISOROCK = ARROW + 1,
	GRASS = ISOROCK + 1,
	LOW_HEALTH_BLOOD = GRASS + 1,
	ENEMY1 = LOW_HEALTH_BLOOD + 1,
	ENEMY1_DMG1 = ENEMY1 + 1,
	ENEMY1_DMG2 = ENEMY1_DMG1 + 1,
	ENEMY1_DMG3 = ENEMY1_DMG2 + 1,
	TEXTURE_COUNT = ENEMY1_DMG3 + 1
};
const int texture_count = (int)TEXTURE_ASSET_ID::TEXTURE_COUNT;

enum class EFFECT_ASSET_ID {
	COLOURED = 0,
	TEXTURED = COLOURED + 1,
	SCREEN = TEXTURED + 1,
	TILED = SCREEN + 1,
	HEALTHBAR = TILED + 1,
	PARTICLE = HEALTHBAR + 1,
	TRAIL = PARTICLE + 1,
	GRASS_BACKGROUND = TRAIL + 1,
	EFFECT_COUNT = GRASS_BACKGROUND + 1
};
const int effect_count = (int)EFFECT_ASSET_ID::EFFECT_COUNT;

enum class GEOMETRY_BUFFER_ID {
	SPRITE = 0,
	BULLET_CIRCLE = SPRITE + 1,
	ENEMY_TRIANGLE = BULLET_CIRCLE + 1,
	ARROW_TRIANGLE = ENEMY_TRIANGLE + 1,
	SCREEN_TRIANGLE = ARROW_TRIANGLE + 1,
	BACKGROUND_QUAD = SCREEN_TRIANGLE + 1,
	FULLSCREEN_QUAD = BACKGROUND_QUAD + 1,
	HEALTH_BAR = FULLSCREEN_QUAD + 1,
	GEOMETRY_COUNT = HEALTH_BAR + 1,
};
const int geometry_count = (int)GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;

struct RenderRequest {
	TEXTURE_ASSET_ID used_texture = TEXTURE_ASSET_ID::TEXTURE_COUNT;
	EFFECT_ASSET_ID used_effect = EFFECT_ASSET_ID::EFFECT_COUNT;
	GEOMETRY_BUFFER_ID used_geometry = GEOMETRY_BUFFER_ID::GEOMETRY_COUNT;
};

// Internal representation of a world chunk
enum class CHUNK_CELL_STATE : char {
	EMPTY = 0,
	ISO_01 = EMPTY + 1,
	ISO_02 = ISO_01 + 1,
	ISO_03 = ISO_02 + 1,
	ISO_04 = ISO_03 + 1,
	ISO_05 = ISO_04 + 1,
	ISO_06 = ISO_05 + 1,
	ISO_07 = ISO_06 + 1,
	ISO_08 = ISO_07 + 1,
	ISO_09 = ISO_08 + 1,
	ISO_10 = ISO_09 + 1,
	ISO_11 = ISO_10 + 1,
	ISO_12 = ISO_11 + 1,
	ISO_13 = ISO_12 + 1,
	ISO_14 = ISO_13 + 1,
	ISO_15 = ISO_14 + 1,
	OBSTACLE = ISO_15 + 1,
	NO_OBSTACLE_AREA = OBSTACLE + 1
};

// Data needed to regenerate a tree obstacle
struct SerializedTree
{
	vec2 position = {0, 0};
	float scale = 1;
};

// Inactive, generated chunk of the game world
struct SerializedChunk
{
	std::vector<SerializedTree> serial_trees;
};

// data for an isoline obstacle
struct IsolineData
{
	vec2 position;
	CHUNK_CELL_STATE state;
	std::vector<Entity> collision_entities;
};

// Chunk of the game world
struct Chunk
{
	std::vector<std::vector<CHUNK_CELL_STATE>> cell_states;
	std::vector<Entity> persistent_entities;
	std::vector<IsolineData> isoline_data;
};

// Obstacles which cross into a chunk (from other chunks)
// Stored in a serialized format to avoid problems with inactive chunks
struct ChunkBoundary
{
	std::vector<SerializedTree> serial_trees;
};
