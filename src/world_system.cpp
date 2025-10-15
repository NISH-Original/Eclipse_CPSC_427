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

void WorldSystem::init(RenderSystem* renderer_arg, InventorySystem* inventory_arg) {
	this->renderer = renderer_arg;
	this->inventory_system = inventory_arg;

	// Pass window handle to inventory system for cursor management
	if (inventory_system && window) {
		inventory_system->set_window(window);
	}

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
	float salmon_vel = 200.0f;

	if (left_pressed && right_pressed) {
		motion.velocity.x = prioritize_right ? salmon_vel : -salmon_vel;
	} else if (left_pressed) {
		motion.velocity.x = -salmon_vel;
	} else if (right_pressed) {
		motion.velocity.x = salmon_vel;
	} else {
		motion.velocity.x = 0.0f;
	}

	if (up_pressed && down_pressed) {
		motion.velocity.y = prioritize_down ? salmon_vel : -salmon_vel;
	} else if (up_pressed) {
		motion.velocity.y = -salmon_vel;
	} else if (down_pressed) {
		motion.velocity.y = salmon_vel;
	} else {
		motion.velocity.y = -0.0f;
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

	return true;
}

// Generate a section of the world
void WorldSystem::generate_chunk(vec2 chunk_pos, Entity player) {
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
	printf("Debug info on world generation:\n");
	printf("   %i valid cells\n", eligible_cells.size());
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
		
		createTree(renderer, pos);

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
	// Debugging for memory/component leaks
	registry.list_all_components();
	printf("Restarting\n");

	// Reset the game speed
	current_speed = 1.f;

	// Remove all entities that we created
	// All that have a motion
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());

	// Debugging for memory/component leaks
	registry.list_all_components();

	// create a new Player
	player_salmon = createPlayer(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});

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
			// TODO: deplete player's health?
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
	if (action == GLFW_RELEASE && key == GLFW_KEY_R) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

        restart_game();
	}

	// M1 TEST: regenerate the world
	if (action == GLFW_RELEASE && key == GLFW_KEY_G) {
		// clear obstacles
		for (Entity entity : registry.obstacles.entities) {
			registry.remove_all_components_of(entity);
		}
		// regenerate obstacles
        generate_chunk(vec2(0, 0), player_salmon);
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
		// Get mouse position
		auto& motion = registry.motions.get(player_salmon);

		float player_diameter = motion.scale.x; // same as width
		float bullet_velocity = 750;

		createBullet(renderer, { motion.position.x + player_diameter * cos(motion.angle), motion.position.y + player_diameter * sin(motion.angle) },
		{ bullet_velocity * cos(motion.angle), bullet_velocity * sin(motion.angle) });
	}

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_button(button, action, mods);
	}
}
