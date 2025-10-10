// Header
#include "world_system.hpp"
#include "world_init.hpp"

// stlib
#include <cassert>
#include <sstream>

#include "physics_system.hpp"

// Game configuration
const size_t CHUNK_CELL_SIZE = 20;
const size_t CHUNK_CELLS_PER_ROW = (size_t) window_width_px / CHUNK_CELL_SIZE;
const size_t CHUNK_CELLS_PER_COLUMN = (size_t) window_height_px / CHUNK_CELL_SIZE;
const int TREES_PER_CHUNK = 40;
const float TREE_SCALE = 40.0f;

// create the underwater world
WorldSystem::WorldSystem()
	: points(0)
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
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);


	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg) {
	this->renderer = renderer_arg;

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
	printf("   Player x min/max: %f and %f", p_min_x, p_max_x);
	printf("   Player y min/max: %f and %f", p_min_y, p_max_y);

	// place trees
	for (int i = 0; i < TREES_PER_CHUNK; i++) {
		size_t n_cell = (size_t) (uniform_dist(rng) * eligible_cells.size());
		std::pair<float, float> selected_cell = eligible_cells[n_cell];
		float pos_x = (selected_cell.first + uniform_dist(rng)) * cell_size;
		float pos_y = (selected_cell.second + uniform_dist(rng)) * cell_size;
		vec2 pos(chunk_pos.x * cells_per_row * cell_size + pos_x,
			     chunk_pos.y * cells_per_col * cell_size + pos_y);
		
		createTree(renderer, pos, {TREE_SCALE, TREE_SCALE});

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

	// generate world
	generate_chunk(vec2(0, 0), player_salmon);
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];

		// for now, we are only interested in collisions that involve the salmon
		if (registry.players.has(entity)) {
			//Player& player = registry.players.get(entity);

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

	auto& motion = registry.motions.get(player_salmon);
	float salmon_vel = 100.0f;

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_S) {
		motion.velocity.y = salmon_vel;
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_S) {
		motion.velocity.y = 0.0f;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_W) {
		motion.velocity.y = -salmon_vel;
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_W) {
		motion.velocity.y = 0.0f;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_A) {
		motion.velocity.x = -salmon_vel;
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_A) {
		motion.velocity.x = 0.0f;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_D) {
		motion.velocity.x = salmon_vel;
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_D) {
		motion.velocity.x = 0.0f;
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

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
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

	(vec2)mouse_position; // dummy to avoid compiler warning
}
