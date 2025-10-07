// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>

#include "physics_system.hpp"

// Game configuration
const size_t MAX_NUM_EELS = 15;
const size_t MAX_NUM_FISH = 5;
const size_t MAX_NUM_BUBBLES = 3;
const size_t MAX_NUM_PINK_FISH = 2;
const size_t EEL_SPAWN_DELAY_MS = 2000 * 3;
const size_t FISH_SPAWN_DELAY_MS = 5000 * 3;
const size_t BUBBLE_SPAWN_DELAY_MS = 3000 * 3;
const size_t PINK_FISH_SPAWN_DELAY_MS = 4000 * 3;

// create the underwater world
WorldSystem::WorldSystem()
	: points(0)
	, next_eel_spawn(0.f)
	, next_fish_spawn(0.f)
	, next_bubble_spawn(0.f)
	, next_pink_fish_spawn(0.f)
	, alive(true) 
	, advanced(true) 
	, right_mom(false)
	, left_mom(false)
	, up_mom(false)
	, down_mom(false) {
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	
	// destroy music components
	if (background_music != nullptr)
		Mix_FreeMusic(background_music);
	if (salmon_dead_sound != nullptr)
		Mix_FreeChunk(salmon_dead_sound);
	if (salmon_eat_sound != nullptr)
		Mix_FreeChunk(salmon_eat_sound);

	Mix_CloseAudio();

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
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);

	//////////////////////////////////////
	// Loading music and sounds with SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Failed to initialize SDL Audio");
		return nullptr;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
		fprintf(stderr, "Failed to open audio device");
		return nullptr;
	}

	background_music = Mix_LoadMUS(audio_path("music.wav").c_str());
	salmon_dead_sound = Mix_LoadWAV(audio_path("death_sound.wav").c_str());
	salmon_eat_sound = Mix_LoadWAV(audio_path("eat_sound.wav").c_str());

	if (background_music == nullptr || salmon_dead_sound == nullptr || salmon_eat_sound == nullptr) {
		fprintf(stderr, "Failed to load sounds\n %s\n %s\n %s\n make sure the data directory is present",
			audio_path("music.wav").c_str(),
			audio_path("death_sound.wav").c_str(),
			audio_path("eat_sound.wav").c_str());
		return nullptr;
	}

	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;
	// Playing background music indefinitely
	Mix_PlayMusic(background_music, -1);
	fprintf(stderr, "Loaded music\n");

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

	// Remove entities that leave the screen on the left side
	// Iterate backwards to be able to remove without unterfering with the next object to visit
	// (the containers exchange the last element with the current)
	for (int i = (int)motions_registry.components.size()-1; i>=0; --i) {
	    Motion& motion = motions_registry.components[i];
		if (motion.position.x + abs(motion.scale.x) < 0.f) {
			if(!registry.players.has(motions_registry.entities[i])) // don't remove the player
				registry.remove_all_components_of(motions_registry.entities[i]);
		}
	}

	// spawn new eels
	next_eel_spawn -= elapsed_ms_since_last_update * current_speed;
	if (registry.deadlys.components.size() <= MAX_NUM_EELS && next_eel_spawn < 0.f) {
		// reset timer
		next_eel_spawn = (EEL_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (EEL_SPAWN_DELAY_MS / 2);

		// create Eel with random initial position
        createEel(renderer, vec2(window_width_px + 50.f, 50.f + uniform_dist(rng) * (window_height_px - 100.f)));
	}

	// spawn fish
	next_fish_spawn -= elapsed_ms_since_last_update * current_speed;
	if (registry.eatables.components.size() <= MAX_NUM_FISH && next_fish_spawn < 0.f) {
		// !!!  TODO A1: create new fish with createFish({0,0}), see eels above
		// reset timer
		next_fish_spawn = (FISH_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (FISH_SPAWN_DELAY_MS / 2);

		// create Fish with random initial position
        createFish(renderer, vec2(window_width_px + 50.f, 50.f + uniform_dist(rng) * (window_height_px - 100.f)));
	}


	// spawn bubbles and pink fish if in advanced mode
	if (advanced) {
		next_bubble_spawn -= elapsed_ms_since_last_update * current_speed;
		if (next_bubble_spawn < 0.f) {
			// reset timer
			next_bubble_spawn = (BUBBLE_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (BUBBLE_SPAWN_DELAY_MS / 2);
	
			// create Bubble with random initial position below screen
			createBubble(renderer, vec2(50.f + uniform_dist(rng) * (window_width_px - 100.f), window_height_px + 50.f));
		}

		next_pink_fish_spawn -= elapsed_ms_since_last_update * current_speed;
		if (next_pink_fish_spawn < 0.f) {
			// reset timer
			next_pink_fish_spawn = (PINK_FISH_SPAWN_DELAY_MS / 2) + uniform_dist(rng) * (PINK_FISH_SPAWN_DELAY_MS / 2);

			// create PinkFish with random initial position on right side
			createPinkFish(renderer, vec2(window_width_px + 50.f, 50.f + uniform_dist(rng) * (window_height_px - 100.f)));
		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A2: HANDLE EGG SPAWN HERE
	// DON'T WORRY ABOUT THIS UNTIL ASSIGNMENT 2
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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

	// !!! TODO A1: update LightUp timers and remove if time drops below zero, similar to the death counter

	for (Entity entity : registry.lightUps.entities) {
		LightUp& counter = registry.lightUps.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;

		if (counter.counter_ms < 0) {
			registry.lightUps.remove(entity);
		} 

		// for debugging
		std::cout << "Entity with ID " << (uint32_t)entity << " lightUp timer: "<< counter.counter_ms << std::endl;
	}

	// update pink fish velocities (randomized)
	for (Entity entity : registry.pinkFishes.entities) {
		PinkFish& pinkFish = registry.pinkFishes.get(entity);
		Motion& motion = registry.motions.get(entity);
		
		pinkFish.velocity_change_timer -= elapsed_ms_since_last_update;
		
		if (pinkFish.velocity_change_timer <= 0) {
			// reset timer
			pinkFish.velocity_change_timer = pinkFish.velocity_change_interval;
			
			// generate new velocity
			std::uniform_real_distribution<float> horizontal_dist(-100.0f, 0.0f);
			std::uniform_real_distribution<float> vertical_dist(-100.0f, 100.0f);
			
			motion.velocity = { horizontal_dist(rng), vertical_dist(rng) };
		}
	}
	
	float step_seconds = elapsed_ms_since_last_update / 1000.0f;

	// Access the salmon entity for advanced momentum
	if (registry.players.has(player_salmon) && advanced) {
		Motion& motion = registry.motions.get(player_salmon);
		
		// X-axis momentum
		if (right_mom) {
			motion.velocity.x = std::min(100.0f, motion.velocity.x + 100.0f * step_seconds);
		} else if (left_mom) {
			motion.velocity.x = std::max(-100.0f, motion.velocity.x - 100.0f * step_seconds);
		} else {
			// X velocity to 0
			if (motion.velocity.x > 0) {
				motion.velocity.x = std::max(0.0f, motion.velocity.x - 25.0f * step_seconds);
			} else if (motion.velocity.x < 0) {
				motion.velocity.x = std::min(0.0f, motion.velocity.x + 25.0f * step_seconds);
			}
		}
		
		// Y-axis momentum
		if (up_mom) {
			motion.velocity.y = std::max(-100.0f, motion.velocity.y - 100.0f * step_seconds);
		} else if (down_mom) {
			motion.velocity.y = std::min(100.0f, motion.velocity.y + 100.0f * step_seconds);
		} else {
			// Y velocity to 0
			if (motion.velocity.y > 0) {
				motion.velocity.y = std::max(0.0f, motion.velocity.y - 25.0f * step_seconds);
			} else if (motion.velocity.y < 0) {
				motion.velocity.y = std::min(0.0f, motion.velocity.y + 25.0f * step_seconds);
			}
		}
	}

	return true;
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	// Debugging for memory/component leaks

	points = 0;
	right_mom = false;
	left_mom = false;
	up_mom = false;
	down_mom = false;
	
	alive = true;

	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that we created
	// All that have a motion, we could also iterate over all fish, eels, ... but that would be more cumbersome
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

	// Debugging for memory/component leaks
	registry.list_all_components();

	// create a new Salmon
	player_salmon = createSalmon(renderer, { 150, 150 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});

	// !! TODO A2: Enable static eggs on the ground, for reference
	// Create eggs on the floor, use this for reference
	/*
	for (uint i = 0; i < 20; i++) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		float radius = 30 * (uniform_dist(rng) + 0.3f); // range 0.3 .. 1.3
		Entity egg = createEgg({ uniform_dist(rng) * w, h - uniform_dist(rng) * 20 },
			         { radius, radius });
		float brightness = uniform_dist(rng) * 0.5 + 0.5;
		registry.colors.insert(egg, { brightness, brightness, brightness});
	}
	*/
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

			// Checking Player - Deadly collisions
			if (registry.deadlys.has(entity_other)) {
				// initiate death unless already dying
				if (!registry.deathTimers.has(entity)) {
					// Scream, reset timer, and make the salmon sink
					registry.deathTimers.emplace(entity);
					Mix_PlayChannel(-1, salmon_dead_sound, 0);

					// !!! TODO A1: change the salmon's orientation and color on death
					right_mom = false;
					left_mom = false;
					up_mom = false;
					down_mom = false;
					alive = false;
					auto& motion = registry.motions.get(entity);
					motion.angle = 0;
					motion.velocity = {0, 100.0f};

					// make salmon red after a collision (2a i)
					registry.colors.get(entity) = {1, 0, 0};
				}
			}
			// Checking Player - Eatable collisions
			else if (registry.eatables.has(entity_other)) {
				if (!registry.deathTimers.has(entity)) {
					// chew, count points, and set the LightUp timer
					registry.remove_all_components_of(entity_other);
					Mix_PlayChannel(-1, salmon_eat_sound, 0);
					++points;

					// !!! TODO A1: create a new struct called LightUp in components.hpp and add an instance to the salmon entity by modifying the ECS registry
					if (!registry.lightUps.has(entity)) {
						registry.lightUps.emplace(entity);
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
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE SALMON MOVEMENT HERE
	// key is of 'type' GLFW_KEY_
	// action can be GLFW_PRESS GLFW_RELEASE GLFW_REPEAT
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	auto& motion = registry.motions.get(player_salmon);
	float salmon_vel = 100.0f;

	if (alive) {
		if (advanced) {

			if (action == GLFW_REPEAT && key == GLFW_KEY_RIGHT) { 
				right_mom = true;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_RIGHT) {
				right_mom = false;
			}
			
			if (action == GLFW_REPEAT && key == GLFW_KEY_LEFT) { 
				left_mom = true;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_LEFT) {
				left_mom = false;
			}
			
			if (action == GLFW_REPEAT && key == GLFW_KEY_UP) { 
				up_mom = true;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_UP) {
				up_mom = false;
			}
			
			if (action == GLFW_REPEAT && key == GLFW_KEY_DOWN) { 
				down_mom = true;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_DOWN) {
				down_mom = false;
			}
		} else {
			if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_DOWN) {
				motion.velocity.y = salmon_vel;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_DOWN) {
				motion.velocity.y = 0.0f;
			}
		
			if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_UP) {
				motion.velocity.y = -salmon_vel;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_UP) {
				motion.velocity.y = 0.0f;
			}
		
			if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_LEFT) {
				motion.velocity.x = -salmon_vel;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_LEFT) {
				motion.velocity.x = 0.0f;
			}
		
			if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_RIGHT) {
				motion.velocity.x = salmon_vel;
			} else if (action == GLFW_RELEASE && key == GLFW_KEY_RIGHT) {
				motion.velocity.x = 0.0f;
			}
		}
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_A) {
		advanced = true;
		std::cout << "Swithed to advaned mode." << std::endl;
		restart_game();
	}
	if (action == GLFW_PRESS && key == GLFW_KEY_B) {
		advanced = false;
		std::cout << "Swithed to basic mode." << std::endl;
		restart_game();
	}

	// Resetting game
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

        restart_game();
	}

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Task 1b) Exit the game on Escape key
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
	// TODO A1: HANDLE SALMON ROTATION HERE
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	if (alive) {
		auto& motion = registry.motions.get(player_salmon);
	
		vec2 salmon_pos = motion.position;
		vec2 direction = mouse_position - salmon_pos;
		float angle = atan2(direction.y, direction.x) + M_PI;
	
		// for debugging
		//std::cout << angle << std::endl;
	
		motion.angle = angle;
	}

	(vec2)mouse_position; // dummy to avoid compiler warning
}
