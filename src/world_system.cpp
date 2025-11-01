// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "noise_gen.hpp"

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
#define M_PI_4 0.78539816339744830961  // pi/4
#endif

// create the underwater world
WorldSystem::WorldSystem() :
	points(0),
	left_pressed(false),
	right_pressed(false),
	up_pressed(false),
	down_pressed(false),
	prioritize_right(false),
	prioritize_down(false),
	mouse_pos(vec2(0,0))
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

void WorldSystem::init(RenderSystem* renderer_arg, InventorySystem* inventory_arg, StatsSystem* stats_arg, ObjectivesSystem* objectives_arg, MinimapSystem* minimap_arg, CurrencySystem* currency_arg, AISystem* ai_arg) {
	this->renderer = renderer_arg;
	this->inventory_system = inventory_arg;
	this->stats_system = stats_arg;
	this->objectives_system = objectives_arg;
	this->minimap_system = minimap_arg;
	this->currency_system = currency_arg;

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

	this->map_perlin = PerlinNoiseGenerator();
	this->decorator_perlin = PerlinNoiseGenerator();

	// Set all states to default
    restart_game();
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
	
	// feet motion and animation
	auto& feet_motion = motions_registry.get(player_feet);
	auto& feet_sprite = registry.sprites.get(player_feet);
	auto& feet_render_request = registry.renderRequests.get(player_feet);
	
	float salmon_vel = 200.0f;

	bool is_moving = false;

	if (left_pressed && right_pressed) {
		motion.velocity.x = prioritize_right ? salmon_vel : -salmon_vel;
		is_moving = true;
	} else if (left_pressed) {
		motion.velocity.x = -salmon_vel;
		is_moving = true;
	} else if (right_pressed) {
		motion.velocity.x = salmon_vel;
		is_moving = true;
	} else {
		motion.velocity.x = 0.0f;
	}

	if (up_pressed && down_pressed) {
		motion.velocity.y = prioritize_down ? salmon_vel : -salmon_vel;
		is_moving = true;
	} else if (up_pressed) {
		motion.velocity.y = -salmon_vel;
		is_moving = true;
	} else if (down_pressed) {
		motion.velocity.y = salmon_vel;
		is_moving = true;
	} else {
		motion.velocity.y = -0.0f;
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
			render_request.used_texture = sprite.previous_animation;
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
			render_request.used_texture = sprite.previous_animation;
		}
	}
	
    if (!sprite.is_shooting && !sprite.is_reloading) {
		if (is_moving && sprite.current_animation != TEXTURE_ASSET_ID::PLAYER_MOVE) {
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_MOVE;
			sprite.total_frame = sprite.move_frames;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_MOVE;
		} else if (!is_moving && sprite.current_animation != TEXTURE_ASSET_ID::PLAYER_IDLE) {
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;
			sprite.total_frame = sprite.idle_frames;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_IDLE;
		}
	}
	
	// feet position follow player
	vec2 local_offset = { 0.f, 5.f };
	float c = cos(motion.angle), s = sin(motion.angle);
	vec2 rotated = { local_offset.x * c - local_offset.y * s,
					local_offset.x * s + local_offset.y * c };
	feet_motion.position = motion.position + rotated;
	feet_motion.angle = motion.angle;
	
    // use walk spritesheet, pause when not moving
    feet_render_request.used_texture = TEXTURE_ASSET_ID::FEET_WALK;
    feet_sprite.total_frame = 20;
    if (is_moving) {
        feet_sprite.animation_speed = 10.0f;
    } else {
        feet_sprite.animation_speed = 0.0f; // pause at current frame
    }
	
	vec2 direction = mouse_pos - motion.position;
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

		// restart the game once the death timer expired
		if (counter.counter_ms < 0) {
			registry.deathTimers.remove(entity);
			screen.darken_screen_factor = 0;
            restart_game();
			return true;
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

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	current_speed = 1.f;
	survival_time_ms = 0.f;
	kill_count = 0;

	// re-seed perlin noise generators
	unsigned int max_seed = ((((unsigned int) (1 << 31) - 1) << 1) + 1);
	unsigned int map_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
	unsigned int decorator_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
	map_perlin.init(map_seed);
	decorator_perlin.init(decorator_seed);

	// Remove all entities that we created
	// All that have a motion
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());
	registry.chunks.clear();

	// Debugging for memory/component leaks
	registry.list_all_components();

	// create a new Player
	player_salmon = createPlayer(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});
	registry.damageCooldowns.emplace(player_salmon); // Add damage cooldown to player

	// create feet for the player
	player_feet = createFeet(renderer, { window_width_px/2, window_height_px - 200 }, player_salmon);

	// draw player after feet
	if (registry.renderRequests.has(player_salmon)) {
		RenderRequest rr = registry.renderRequests.get(player_salmon);
		registry.renderRequests.remove(player_salmon);
		registry.renderRequests.insert(player_salmon, rr);
	}

	flashlight = createFlashlight(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(flashlight, {1, 1, 1});
	
	Light& flashlight_light = registry.lights.get(flashlight);
	flashlight_light.follow_target = player_salmon;

	// Initialize player inventory
	if (inventory_system) {
		inventory_system->init_player_inventory(player_salmon);
	}

	// generate world
	generate_chunk(vec2(0, 0), map_perlin);

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
			enemy.is_dead = true;
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
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_S) {
		down_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_W) {
		up_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_down = false;
		}
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_W) {
		up_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_A) {
		left_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_right = false;
		}
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_A) {
		left_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_D) {
		right_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_right = true;
		}
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
		// clear chunks
		for (int i = 0; i < registry.chunks.size(); i++) {
			Chunk& chunk = registry.chunks.components[i];
			for (int j = 0; j < chunk.persistent_entities.size(); j++) {
				registry.remove_all_components_of(chunk.persistent_entities[j]);
			}
			registry.chunks.remove(registry.chunks.position_xs[i], registry.chunks.position_ys[i]);
		}

		// reseed noise generators
		unsigned int max_seed = ((((unsigned int) (1 << 31) - 1) << 1) + 1);
		unsigned int map_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
		unsigned int decorator_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
		map_perlin.init(map_seed);
		decorator_perlin.init(decorator_seed);

		// regenerate chunks
        generate_chunk(vec2(0, 0), map_perlin);
	}

	// Toggle inventory with I key
	if (action == GLFW_RELEASE && key == GLFW_KEY_I) {
		if (inventory_system) {
			inventory_system->toggle_inventory();
		}
	}

	// Cmd+R (Mac) or Ctrl+R: Hot reload UI (for development)
    if (action == GLFW_RELEASE && key == GLFW_KEY_R && (mod & GLFW_MOD_SUPER || mod & GLFW_MOD_CONTROL)) {
		if (inventory_system && inventory_system->is_inventory_open()) {
			std::cout << "⌘+R pressed - manually reloading UI..." << std::endl;
			inventory_system->reload_ui();
		}
	}

    // R to reload weapon (if not full and not already reloading or shooting)
    if (action == GLFW_RELEASE && key == GLFW_KEY_R && !(mod & (GLFW_MOD_SUPER | GLFW_MOD_CONTROL))) {
        if (registry.players.has(player_salmon)) {
            Player& player = registry.players.get(player_salmon);
            Sprite& sprite = registry.sprites.get(player_salmon);
            auto& render_request = registry.renderRequests.get(player_salmon);
            if (!sprite.is_reloading && !sprite.is_shooting && player.ammo_in_mag < player.magazine_size) {
                sprite.is_reloading = true;
                sprite.reload_timer = sprite.reload_duration;
                sprite.previous_animation = sprite.current_animation;
                sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_RELOAD;
                sprite.total_frame = sprite.reload_frames;
                sprite.curr_frame = 0;
                sprite.step_seconds_acc = 0.0f;
                render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_RELOAD;
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

	// Toggle occlusion mask visualization with 'O' key
	if (action == GLFW_PRESS && key == GLFW_KEY_O) {
		// Changing the brightness for now, as the occlusion mask looks a bit glitchy with obstacles & enemies with sprites
		renderer->setGlobalAmbientBrightness(1.0f - renderer->global_ambient_brightness);
		// debugging.show_occlusion_mask = !debugging.show_occlusion_mask;
		// printf("Occlusion mask debug: %s\n", debugging.show_occlusion_mask ? "ON" : "OFF");
	}

	// Exit the game on Escape key
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}

	// Control the current speed with `<` `>`
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
		printf("Current speed = %f\n", current_speed);
	}
	current_speed = fmax(0.f, current_speed);
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	mouse_pos = mouse_position;

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_move(mouse_position);
	}

	// for debugging
	//std::cout << angle << std::endl;
}

void WorldSystem::on_mouse_click(int button, int action, int mods) {

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		// Get player components
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
			render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_SHOOT;

			float player_diameter = motion.scale.x; // same as width
			float bullet_velocity = 750;

			// Calculate bullet spawn position. The hardcoded values here are found by trial and error to spawn the bullets from the muzzle of the gun.
			vec2 bullet_spawn_pos = { 
				motion.position.x + player_diameter * 0.55f * cos(motion.angle + M_PI_4 * 0.6f), 
				motion.position.y + player_diameter * 0.55f * sin(motion.angle + M_PI_4 * 0.6f) 
			};

			// Calculate direction from bullet spawn position to mouse
			vec2 direction = mouse_pos - bullet_spawn_pos;
			float bullet_angle = atan2(direction.y, direction.x);

            createBullet(renderer, bullet_spawn_pos,
            { bullet_velocity * cos(bullet_angle), bullet_velocity * sin(bullet_angle) });

            // decrease ammo
            player.ammo_in_mag = std::max(0, player.ammo_in_mag - 1);
		}
	}

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_button(button, action, mods);
	}
}
