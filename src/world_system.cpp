// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>

#include "physics_system.hpp"
#include "ai_system.hpp"

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923  // pi/2
#endif

#ifndef M_PI_4
#define M_PI_4 0.78539816339744830962
#endif

// Game configuration
const size_t CHUNK_CELL_SIZE = 20;
const size_t CHUNK_CELLS_PER_ROW = (size_t) window_width_px / CHUNK_CELL_SIZE;
const size_t CHUNK_CELLS_PER_COLUMN = (size_t) window_height_px / CHUNK_CELL_SIZE;
const int TREES_PER_CHUNK = 20;

// create the underwater world
WorldSystem::WorldSystem() :
	points(0),
	left_pressed(false),
	right_pressed(false),
	up_pressed(false),
	down_pressed(false),
	prioritize_right(false),
	prioritize_down(false),
	mouse_pos(vec2(0,0)),
	is_dashing(false),
	dash_timer(0.0f),
	dash_cooldown_timer(0.0f),
	is_knockback(false),
	knockback_timer(0.0f),
	knockback_direction(vec2(0,0))
{
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}
}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_width_px, window_height_px, "Salmon Game Assignment", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_click(_0, _1, _2); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_redirect);


	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg, InventorySystem* inventory_arg, StatsSystem* stats_arg, ObjectivesSystem* objectives_arg, MinimapSystem* minimap_arg, CurrencySystem* currency_arg, TutorialSystem* tutorial_arg, AISystem* ai_arg, AudioSystem* audio_arg) {
	this->renderer = renderer_arg;
	this->inventory_system = inventory_arg;
	this->stats_system = stats_arg;
	this->objectives_system = objectives_arg;
	this->minimap_system = minimap_arg;
	this->currency_system = currency_arg;
	this->tutorial_system = tutorial_arg;
	this->audio_system = audio_arg;

	// Pass window handle to inventory system for cursor management
	if (inventory_system && window) {
		inventory_system->set_window(window);
	}
	
	// Set up kill callback for AI system
	if (ai_arg) {
		ai_arg->set_kill_callback([this]() {
			this->kill_count++;
		});
	}

	// Set all states to default
    restart_game();
    
    // Start the tutorial when game begins
    if (tutorial_system) {
    	tutorial_system->start_tutorial();
    }
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
	    registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Handle player motion
	auto& motion = motions_registry.get(player_salmon);
	auto& sprite = registry.sprites.get(player_salmon);
	auto& render_request = registry.renderRequests.get(player_salmon);

	renderer->setCameraPosition(motion.position);

	// feet motion and animation
	auto& feet_motion = motions_registry.get(player_feet);
	auto& feet_sprite = registry.sprites.get(player_feet);
	auto& feet_render_request = registry.renderRequests.get(player_feet);

	// flashlight motion
	auto& flashlight_motion = motions_registry.get(flashlight);
	
	float salmon_vel = 200.0f;

	// update dash timers
	float elapsed_seconds = elapsed_ms_since_last_update / 1000.0f;
	if (is_dashing) {
		dash_timer -= elapsed_seconds;
		if (dash_timer <= 0.0f) {
			is_dashing = false;
			dash_timer = 0.0f;
			dash_cooldown_timer = dash_cooldown;
			dash_direction = {0.0f, 0.0f};
		}
	} else if (dash_cooldown_timer > 0.0f) {
		dash_cooldown_timer -= elapsed_seconds;
		if (dash_cooldown_timer < 0.0f) {
			dash_cooldown_timer = 0.0f;
		}
	}

	// Update knockback timers
	if (is_knockback) {
		knockback_timer -= elapsed_seconds;
		if (knockback_timer <= 0.0f) {
			is_knockback = false;
			knockback_timer = 0.0f;
			knockback_direction = {0.0f, 0.0f}; // reset knockback direction
		}
	}

	bool is_moving = false;


	if (is_knockback) {
		// lock velocity to knockback direction
		motion.velocity.x = knockback_direction.x * salmon_vel * knockback_multiplier;
		motion.velocity.y = knockback_direction.y * salmon_vel * knockback_multiplier;
		is_moving = true;
	} else if (is_dashing) {
		// lock velocity to dash direction
		motion.velocity.x = dash_direction.x * salmon_vel * dash_multiplier;
		motion.velocity.y = dash_direction.y * salmon_vel * dash_multiplier;
		is_moving = true;
	} else {
		// normal movement
		float current_vel = salmon_vel;

		if (left_pressed && right_pressed) {
			motion.velocity.x = prioritize_right ? current_vel : -current_vel;
			is_moving = true;
		} else if (left_pressed) {
			motion.velocity.x = -current_vel;
			is_moving = true;
		} else if (right_pressed) {
			motion.velocity.x = current_vel;
			is_moving = true;
		} else {
			motion.velocity.x = 0.0f;
		}

		if (up_pressed && down_pressed) {
			motion.velocity.y = prioritize_down ? current_vel : -current_vel;
			is_moving = true;
		} else if (up_pressed) {
			motion.velocity.y = -current_vel;
			is_moving = true;
		} else if (down_pressed) {
			motion.velocity.y = current_vel;
			is_moving = true;
		} else {
			motion.velocity.y = -0.0f;
		}
	}

	// Handle shooting animation
	if (sprite.is_shooting) {
		sprite.shoot_timer -= elapsed_ms_since_last_update / 1000.0f;
		
		if (sprite.shoot_timer <= 0.0f) {
			// return to previous state
			sprite.is_shooting = false;
			sprite.current_animation = sprite.previous_animation;
			
			if (sprite.previous_animation == TEXTURE_ASSET_ID::PLAYER_MOVE) {
				sprite.total_frame = sprite.move_frames;
			} else {
				sprite.total_frame = sprite.idle_frames;
			}
			
			sprite.curr_frame = 0; // reset animation
			sprite.step_seconds_acc = 0.0f; // reset timer
			render_request.used_texture = get_weapon_texture(sprite.previous_animation);
		}
	}

	// reload animation
	if (sprite.is_reloading) {
		sprite.reload_timer -= elapsed_ms_since_last_update / 1000.0f;
		if (sprite.reload_timer <= 0.0f) {
			// finish reload
			sprite.is_reloading = false;
			// refill ammo
			if (registry.players.has(player_salmon)) {
				Player& player = registry.players.get(player_salmon);
				player.ammo_in_mag = player.magazine_size;
			}
			// restore animation
			sprite.current_animation = sprite.previous_animation;
			if (sprite.previous_animation == TEXTURE_ASSET_ID::PLAYER_MOVE) {
				sprite.total_frame = sprite.move_frames;
			} else {
				sprite.total_frame = sprite.idle_frames;
			}
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = get_weapon_texture(sprite.previous_animation);
		}
	}
	
    if (!sprite.is_shooting && !sprite.is_reloading) {
		if (is_moving && sprite.current_animation != TEXTURE_ASSET_ID::PLAYER_MOVE) {
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_MOVE;
			sprite.total_frame = sprite.move_frames;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_MOVE);
		} else if (!is_moving && sprite.current_animation != TEXTURE_ASSET_ID::PLAYER_IDLE) {
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;
			sprite.total_frame = sprite.idle_frames;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_IDLE);
		}
	}
	
	// feet position follow player
	vec2 feet_offset = { 0.f, 5.f };
	float c = cos(motion.angle), s = sin(motion.angle);
	vec2 feet_rotated = { feet_offset.x * c - feet_offset.y * s,
						  feet_offset.x * s + feet_offset.y * c };
	feet_motion.position = motion.position + feet_rotated;
	feet_motion.angle = motion.angle;

	// flashlight position follow player
	vec2 flashlight_offset = { 50.0f, 25.0f };
	vec2 flashlight_rotated = { flashlight_offset.x * c - flashlight_offset.y * s,
								flashlight_offset.x * s + flashlight_offset.y * c };
	flashlight_motion.position = motion.position + flashlight_rotated;
	flashlight_motion.angle = motion.angle;

    // use walk spritesheet, pause when not moving
    feet_render_request.used_texture = TEXTURE_ASSET_ID::FEET_WALK;
    feet_sprite.total_frame = 20;
    if (is_moving) {
        feet_sprite.animation_speed = 10.0f;
    } else {
        feet_sprite.animation_speed = 0.0f; // pause at current frame
    }
	
	// calculate the angle of the mouse relative to the player
	vec2 world_mouse_pos;
	world_mouse_pos.x = mouse_pos.x - (window_width_px / 2.0f) + motion.position.x;
	world_mouse_pos.y = mouse_pos.y - (window_height_px / 2.0f) + motion.position.y;

	vec2 direction = world_mouse_pos - motion.position;
	float angle = atan2(direction.y, direction.x);
	motion.angle = angle;

	// Remove entities that leave the screen on any side
	for (int i = (int)motions_registry.components.size()-1; i>=0; --i) {
	    Motion& motion = motions_registry.components[i];
		Entity entity = motions_registry.entities[i];
		
		// Don't remove the player
		if(registry.players.has(entity)) continue;
		
		// Check all screen boundaries
		if (motion.position.x + abs(motion.scale.x) < 0.f ||
			motion.position.x - abs(motion.scale.x) > window_width_px ||
			motion.position.y + abs(motion.scale.y) < 0.f ||
			motion.position.y - abs(motion.scale.y) > window_height_px) {
			registry.remove_all_components_of(entity);
		}
	}

	// Processing the salmon state
	assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    float min_counter_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		// progress timer
		DeathTimer& counter = registry.deathTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if(counter.counter_ms < min_counter_ms){
		    min_counter_ms = counter.counter_ms;
		}

		if (counter.counter_ms < 0) {
			registry.deathTimers.remove(entity);
			if (registry.players.has(entity)) {
				screen.darken_screen_factor = 0;
				restart_game();
				return true;
			} else {
				registry.remove_all_components_of(entity);
			}
		}
	}
	// reduce window brightness if the salmon is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	auto& cooldowns = registry.damageCooldowns;
	for (uint i = 0; i < cooldowns.components.size(); i++) {
		DamageCooldown& cooldown = cooldowns.components[i];
		if (cooldown.cooldown_ms > 0) {
			cooldown.cooldown_ms -= elapsed_ms_since_last_update;
		}
	}

	if (stats_system && registry.players.has(player_salmon)) {
		stats_system->update_player_stats(player_salmon);
	}
	
	survival_time_ms += elapsed_ms_since_last_update;
	
	const float SPAWN_RADIUS = 800.0f;
	
	if (objectives_system) {
		float survival_seconds = survival_time_ms / 1000.0f;
		bool survival_complete = survival_seconds >= 180.0f;
		char survival_text[64];
		snprintf(survival_text, sizeof(survival_text), "Survival: %.0fs / 180s", survival_seconds);
		objectives_system->set_objective(1, survival_complete, survival_text);
		
		bool kill_complete = kill_count >= 25;
		char kill_text[64];
		snprintf(kill_text, sizeof(kill_text), "Kill: %d / 25", kill_count);
		objectives_system->set_objective(2, kill_complete, kill_text);
		
		Motion& player_motion = registry.motions.get(player_salmon);
		float distance_from_spawn = sqrt(player_motion.position.x * player_motion.position.x + 
		                                   player_motion.position.y * player_motion.position.y);
		bool exit_radius_complete = distance_from_spawn > SPAWN_RADIUS;
		objectives_system->set_objective(3, exit_radius_complete, "Exit spawn radius");
	}
	
	if (minimap_system && registry.players.has(player_salmon)) {
		vec2 spawn_position = { window_width_px/2.0f, window_height_px - 200.0f };
		minimap_system->update_player_position(player_salmon, SPAWN_RADIUS, spawn_position);
	}
	
	if (currency_system && registry.players.has(player_salmon)) {
		Player& player = registry.players.get(player_salmon);
		currency_system->update_currency(player.currency);
	}

	return true;
}

// Generate a section of the world
void WorldSystem::generate_chunk(vec2 chunk_pos, Entity player) {
	// check if chunk has already been generated
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunks.has(chunk_pos_x, chunk_pos_y))
		return;

	// initialize new chunk
	Chunk& chunk = registry.chunks.emplace(chunk_pos_x, chunk_pos_y);
	chunk.cell_states.resize(CHUNK_CELLS_PER_ROW);

	float cell_size = (float) CHUNK_CELL_SIZE;
	float cells_per_row = (float) CHUNK_CELLS_PER_ROW;
	float cells_per_col = (float) CHUNK_CELLS_PER_COLUMN;

	Motion& p_motion = registry.motions.get(player);
	float p_min_x = p_motion.position.x - (abs(p_motion.scale.x) / 2);
	float p_max_x = p_motion.position.x + (abs(p_motion.scale.x) / 2);
	float p_min_y = p_motion.position.y - (abs(p_motion.scale.y) / 2);
	float p_max_y = p_motion.position.y + (abs(p_motion.scale.y) / 2);

	// generate list of eligible positions
	std::vector<std::pair<float, float>> eligible_cells;
	for (float i = 1; i < cells_per_row - 1; i++) {
		for (float j = 1; j < cells_per_col - 1; j++) {
			if ((i+2)*cell_size <= p_min_x || (i-1)*cell_size >= p_max_x ||
				(j+2)*cell_size <= p_min_y || (j-1)*cell_size >= p_max_y) {
				eligible_cells.push_back(std::pair<float, float>(i, j));
			}
		}
	}
	printf("Debug info for generation of chunk (%i, %i):\n", chunk_pos_x, chunk_pos_y);
	printf("   %lu valid cells\n", eligible_cells.size());
	printf("   Player x min/max: %f and %f\n", p_min_x, p_max_x);
	printf("   Player y min/max: %f and %f\n", p_min_y, p_max_y);

	// place trees
	for (int i = 0; i < TREES_PER_CHUNK; i++) {
		size_t n_cell = (size_t) (uniform_dist(rng) * eligible_cells.size());
		std::pair<float, float> selected_cell = eligible_cells[n_cell];
		float pos_x = (selected_cell.first + uniform_dist(rng)) * cell_size;
		float pos_y = (selected_cell.second + uniform_dist(rng)) * cell_size;
		vec2 pos(chunk_pos.x * cells_per_row * cell_size + pos_x,
			     chunk_pos.y * cells_per_col * cell_size + pos_y);
		
		Entity tree = createTree(renderer, pos);
		chunk.persistent_entities.push_back(tree);

		// remove ineligible cells
		for (size_t n = 0; n < eligible_cells.size();) {
			std::pair<float, float> pair = eligible_cells[n];
			float i_diff = abs(pair.first - selected_cell.first);
			float j_diff = abs(pair.second - selected_cell.second);
			if (i_diff <= 2 && j_diff <= 2) {
				std::pair<float, float> last = eligible_cells[eligible_cells.size() - 1];
				eligible_cells[n] = last;
				eligible_cells.pop_back();
			} else {
				n++;
			}
		}
	}
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	current_speed = 1.f;
	survival_time_ms = 0.f;
	kill_count = 0;

	// Remove all entities that we created
	// All that have a motion
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());
	registry.chunks.clear();
	
	// Clear inventories (since player entity will be recreated with new ID)
	registry.inventories.clear();

	// create feet first so it renders under the player
	player_feet = createFeet(renderer, { window_width_px/2, window_height_px - 200 }, Entity());

	// create a new Player
	player_salmon = createPlayer(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});
	registry.damageCooldowns.emplace(player_salmon); // Add damage cooldown to player

	// set parent reference now that player exists
	registry.feet.get(player_feet).parent_player = player_salmon;

	flashlight = createFlashlight(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(flashlight, {1, 1, 1});

	Light& flashlight_light = registry.lights.get(flashlight);
	flashlight_light.follow_target = player_salmon;

	// Initialize player inventory
	if (inventory_system) {
		inventory_system->init_player_inventory(player_salmon);
	}

	// generate world
	generate_chunk(vec2(0, 0), player_salmon);

	// instead of a constant solid background
	// created a quad that can be affected by the lighting
	background = createBackground(renderer);
	registry.colors.insert(background, {0.1f, 0.1f, 0.1f});

	// TODO: remove hardcoded enemy creates
	glm::vec2 player_init_position = { window_width_px/2, window_height_px - 200 };
	createSlime(renderer, { player_init_position.x + 300, player_init_position.y + 150 });
	createSlime(renderer, { player_init_position.x - 300, player_init_position.y + 150 });
	createEnemy(renderer, { player_init_position.x + 300, player_init_position.y - 150 });
	createEnemy(renderer, { player_init_position.x - 300, player_init_position.y - 150 });
	createEnemy(renderer, { player_init_position.x + 350, player_init_position.y });
	createEnemy(renderer, { player_init_position.x - 350, player_init_position.y });
	createSlime(renderer, { player_init_position.x - 100, player_init_position.y - 300 });
	createEnemy(renderer, { player_init_position.x + 100, player_init_position.y - 300 });
}

// get texture based on equipped weapon
TEXTURE_ASSET_ID WorldSystem::get_weapon_texture(TEXTURE_ASSET_ID base_texture) const {
	if (registry.inventories.has(player_salmon)) {
		Inventory& inventory = registry.inventories.get(player_salmon);
		if (registry.weapons.has(inventory.equipped_weapon)) {
			Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
			if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY || 
			    weapon.type == WeaponType::PLASMA_SHOTGUN_UNSTABLE) {
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_IDLE) return TEXTURE_ASSET_ID::SHOTGUN_IDLE;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_MOVE) return TEXTURE_ASSET_ID::SHOTGUN_MOVE;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_SHOOT) return TEXTURE_ASSET_ID::SHOTGUN_SHOOT;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_RELOAD) return TEXTURE_ASSET_ID::SHOTGUN_RELOAD;
			}
		}
	}
	// Default is pistol textures
	return base_texture;
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;
		// for now, we are only interested in collisions that involve the salmon
		if (registry.players.has(entity)) {
			//Player& player = registry.players.get(entity);

		}

		// When enemy was shot by the bullet
		if (registry.enemies.has(entity) && registry.bullets.has(entity_other)) {
			Enemy& enemy = registry.enemies.get(entity);
			Bullet& bullet = registry.bullets.get(entity_other);

			// Subtract bullet damage from enemy health
			enemy.health -= bullet.damage;

			// Check if enemy should die
			if (enemy.health <= 0) {
				enemy.is_dead = true;

				// Award xylarite to player
				Player& player = registry.players.get(player_salmon);
				player.currency += 10; // 10 xylarite per enemy

				// Update currency UI
				if (currency_system) {
					currency_system->update_currency(player.currency);
				}
			}

			// Play impact sound
			if (audio_system) {
				audio_system->play("impact-enemy");
			}

			// Destroy the bullet
			registry.remove_all_components_of(entity_other);
		}

		// When bullet hits an obstacle (tree)
		if (registry.obstacles.has(entity) && registry.bullets.has(entity_other)) {
			// Play tree impact sound
			if (audio_system) {
				audio_system->play("impact-tree");
			}

			// Destroy the bullet
			registry.remove_all_components_of(entity_other);
		}

		// When player is hit by the enemy
		if (registry.enemies.has(entity) && registry.players.has(entity_other)) {
			// Deplete player's health with cooldown
			if (registry.damageCooldowns.has(entity_other)) {
				DamageCooldown& cooldown = registry.damageCooldowns.get(entity_other);
				if (cooldown.cooldown_ms <= 0) {
				Player& player = registry.players.get(entity_other);
				Enemy& enemy = registry.enemies.get(entity);
				player.health -= enemy.damage;
				cooldown.cooldown_ms = cooldown.max_cooldown_ms;
					
					// Check if player is dead
					if (player.health <= 0) {
						player.health = 0;
						restart_game();
					}
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {
	// track which keys are being pressed, for player movement
	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_S) {
		down_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_down = true;
		}
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_S) {
		down_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_W) {
		up_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_down = false;
		}
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_W) {
		up_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_A) {
		left_pressed = true;
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_A) {
		left_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_D) {
		right_pressed = true;
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_D) {
		right_pressed = false;
	}

    // Resetting game
	// Keybind moved to "=" to free "R" for reload and ensure keybind is pressable on newer Macs
    if (action == GLFW_RELEASE && key == GLFW_KEY_EQUAL) {
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        restart_game();
    }

	// M1 TEST: regenerate the world
	if (action == GLFW_RELEASE && key == GLFW_KEY_G) {
		// clear obstacles
		for (int i = 0; i < registry.chunks.size(); i++) {
			Chunk& chunk = registry.chunks.components[i];
			for (int j = 0; j < chunk.persistent_entities.size(); j++) {
				registry.remove_all_components_of(chunk.persistent_entities[j]);
			}
			registry.chunks.remove(registry.chunks.position_xs[i], registry.chunks.position_ys[i]);
		}
		// regenerate obstacles
        generate_chunk(vec2(0, 0), player_salmon);
	}

	if (action == GLFW_RELEASE && key == GLFW_KEY_I) {
		if (tutorial_system && tutorial_system->is_active() && tutorial_system->should_pause()) {
			if (tutorial_system->get_required_action() == TutorialSystem::Action::OpenInventory) {
				tutorial_system->on_next_clicked();
			}
		}
		if (inventory_system) {
			inventory_system->toggle_inventory();
		}
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::OpenInventory);
	}

	// Cmd+R (Mac) or Ctrl+R: Hot reload UI (for development)
    if (action == GLFW_RELEASE && key == GLFW_KEY_R && (mod & GLFW_MOD_SUPER || mod & GLFW_MOD_CONTROL)) {
		if (inventory_system && inventory_system->is_inventory_open()) {
			std::cout << "⌘+R pressed - manually reloading UI..." << std::endl;
			inventory_system->reload_ui();
		}
	}

    if (action == GLFW_RELEASE && key == GLFW_KEY_R && !(mod & (GLFW_MOD_SUPER | GLFW_MOD_CONTROL))) {
        if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Reload);
        if (registry.players.has(player_salmon)) {
            Player& player = registry.players.get(player_salmon);
            Sprite& sprite = registry.sprites.get(player_salmon);
            auto& render_request = registry.renderRequests.get(player_salmon);
            if (!sprite.is_reloading && !sprite.is_shooting && player.ammo_in_mag < player.magazine_size) {
                // determine reload frames based on equipped weapon
                int reload_frame_count = sprite.reload_frames; // default to pistol
                if (registry.inventories.has(player_salmon)) {
                    Inventory& inventory = registry.inventories.get(player_salmon);
                    if (registry.weapons.has(inventory.equipped_weapon)) {
                        Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
                        // shotgun uses 20 frames, pistol uses 15
                        if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY || 
                            weapon.type == WeaponType::PLASMA_SHOTGUN_UNSTABLE) {
                            reload_frame_count = 20;
                        }
                    }
                }
                
                sprite.is_reloading = true;
                sprite.reload_timer = sprite.reload_duration;
                sprite.previous_animation = sprite.current_animation;
                sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_RELOAD;
                sprite.total_frame = reload_frame_count;
                sprite.curr_frame = 0;
                sprite.step_seconds_acc = 0.0f;
                render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_RELOAD);
            }
        }
    }

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Toggle ambient brightness with 'O' key
	if (action == GLFW_PRESS && key == GLFW_KEY_O) {
		renderer->setGlobalAmbientBrightness(1.0f - renderer->global_ambient_brightness);
	}

	// Toggle player hitbox debug with 'C'
	if (action == GLFW_RELEASE && key == GLFW_KEY_C) {
		renderer->togglePlayerHitboxDebug();
	}

	// Dash with SHIFT key, only if moving and cooldown is ready
	if (action == GLFW_PRESS && key == GLFW_KEY_LEFT_SHIFT) {
		if (!is_dashing && dash_cooldown_timer <= 0.0f) {
			float dir_x = 0.0f;
			float dir_y = 0.0f;
			
			if (left_pressed && right_pressed) {
				dir_x = prioritize_right ? 1.0f : -1.0f;
			} else if (left_pressed) {
				dir_x = -1.0f;
			} else if (right_pressed) {
				dir_x = 1.0f;
			}
			if (up_pressed && down_pressed) {
				dir_y = prioritize_down ? 1.0f : -1.0f;
			} else if (up_pressed) {
				dir_y = -1.0f;
			} else if (down_pressed) {
				
				dir_y = 1.0f;
			}
			
			// normalize vector
			float dir_len = sqrtf(dir_x * dir_x + dir_y * dir_y);
			if (dir_len > 0.0001f) {
				dash_direction.x = dir_x / dir_len;
				dash_direction.y = dir_y / dir_len;
				is_dashing = true;
				dash_timer = dash_duration;
			}
		}
	}

	// Exit the game on Escape key
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}

	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
	}
	current_speed = fmax(0.f, current_speed);
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	mouse_pos = mouse_position;

	// Pass mouse position to tutorial for hover effects
	if (tutorial_system && tutorial_system->is_active()) {
		tutorial_system->on_mouse_move(mouse_position);
	}

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_move(mouse_position);
	}
}

void WorldSystem::on_mouse_click(int button, int action, int mods) {
	if (tutorial_system && tutorial_system->should_pause()) {
		tutorial_system->on_mouse_button(button, action, mods);
		return;
	}

	if (tutorial_system && tutorial_system->is_active() && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		tutorial_system->notify_action(TutorialSystem::Action::Shoot);
	}

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_button(button, action, mods);
		return;
	}

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		auto& motion = registry.motions.get(player_salmon);
		auto& sprite = registry.sprites.get(player_salmon);
		auto& render_request = registry.renderRequests.get(player_salmon);
        Player& player = registry.players.get(player_salmon);

        if (!sprite.is_shooting && !sprite.is_reloading && player.ammo_in_mag > 0) {
			sprite.is_shooting = true;
			sprite.shoot_timer = sprite.shoot_duration;
			sprite.previous_animation = sprite.current_animation;
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_SHOOT;
			sprite.total_frame = sprite.shoot_frames;
			sprite.curr_frame = 0; // Reset animation
			sprite.step_seconds_acc = 0.0f; // Reset timer
			render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_SHOOT);

			float player_diameter = motion.scale.x; // same as width
			float bullet_velocity = 750;

			// Calculate bullet spawn position. The hardcoded values here are found by trial and error to spawn the bullets from the muzzle of the gun.
			vec2 bullet_spawn_pos = { 
				motion.position.x + player_diameter * 0.55f * cos(motion.angle + M_PI_4 * 0.6f), 
				motion.position.y + player_diameter * 0.55f * sin(motion.angle + M_PI_4 * 0.6f) 
			};

			vec2 world_mouse_pos;
			world_mouse_pos.x = mouse_pos.x - (window_width_px / 2.0f) + motion.position.x;
			world_mouse_pos.y = mouse_pos.y - (window_height_px / 2.0f) + motion.position.y;

			// Calculate direction from bullet spawn position to mouse
			vec2 direction = world_mouse_pos - bullet_spawn_pos;
			float base_angle = atan2(direction.y, direction.x);

			// check if shotgun is equipped
			bool is_shotgun = false;
			if (registry.inventories.has(player_salmon)) {
				Inventory& inventory = registry.inventories.get(player_salmon);
				if (registry.weapons.has(inventory.equipped_weapon)) {
					Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
					if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY || 
					    weapon.type == WeaponType::PLASMA_SHOTGUN_UNSTABLE) {
						is_shotgun = true;
					}
				}
			}

			// play gunshot sound (shotgun or pistol)
			if (audio_system) {
				if (is_shotgun) {
					audio_system->play("shotgun_gunshot");
				} else {
					audio_system->play("gunshot");
				}
			}

			if (is_shotgun) {
				// Shotgun fires 5 bullets
				float spread_angles[] = { -20.0f, -10.0f, 0.0f, 10.0f, 20.0f };
				float deg_to_rad = M_PI / 180.0f;
				for (int i = 0; i < 5; i++) {
					float bullet_angle = base_angle + spread_angles[i] * deg_to_rad;
					createBullet(renderer, bullet_spawn_pos,
						{ bullet_velocity * cos(bullet_angle), bullet_velocity * sin(bullet_angle) });
				}
				
				// knockback in opposite direction of shooting
				knockback_direction.x = -cos(base_angle);
				knockback_direction.y = -sin(base_angle);
				float dir_len = sqrtf(knockback_direction.x * knockback_direction.x + 
				                     knockback_direction.y * knockback_direction.y);
				if (dir_len > 0.0001f) {
					knockback_direction.x /= dir_len;
					knockback_direction.y /= dir_len;
				}
				is_knockback = true;
				knockback_timer = knockback_duration;
			} else {
				// pistol fires 1 bullet
				createBullet(renderer, bullet_spawn_pos,
					{ bullet_velocity * cos(base_angle), bullet_velocity * sin(base_angle) });
			}

			Entity muzzle_flash = Entity();
			Motion& flash_motion = registry.motions.emplace(muzzle_flash);
			flash_motion.position = bullet_spawn_pos;
			flash_motion.angle = base_angle;
			Light& flash_light = registry.lights.emplace(muzzle_flash);
			flash_light.is_enabled = true;
			flash_light.cone_angle = 2.8f;
			flash_light.brightness = 8.0f;
			flash_light.range = 500.0f;
			flash_light.light_color = { 1.0f, 0.9f, 0.5f };
			DeathTimer& flash_timer = registry.deathTimers.emplace(muzzle_flash);
			flash_timer.counter_ms = 50.0f;

            player.ammo_in_mag = std::max(0, player.ammo_in_mag - 1);
		}
	}
}
