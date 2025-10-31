
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "inventory_system.hpp"
#include "stats_system.hpp"
#include "objectives_system.hpp"
#include "minimap_system.hpp"
#include "currency_system.hpp"
#include "menu_icons_system.hpp"
#include "tutorial_system.hpp"
#include "ai_system.hpp"

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// Global systems
	WorldSystem world;
	RenderSystem renderer;
	PhysicsSystem physics;
	InventorySystem inventory;
	StatsSystem stats;
	ObjectivesSystem objectives;
	MinimapSystem minimap;
	CurrencySystem currency;
	MenuIconsSystem menu_icons;
	TutorialSystem tutorial;
	AISystem ai;

	// Initializing window
	GLFWwindow* window = world.create_window();
	if (!window) {
		// Time to read the error message
		printf("Press any key to exit");
		getchar();
		return EXIT_FAILURE;
	}

	// initialize the main systems
	renderer.init(window);
	inventory.init(window);
	
	stats.init(inventory.get_context());
	objectives.init(inventory.get_context());
	minimap.init(inventory.get_context());
	currency.init(inventory.get_context());
	menu_icons.init(inventory.get_context());
	tutorial.init(inventory.get_context());
	
	world.init(&renderer, &inventory, &stats, &objectives, &minimap, &currency, &tutorial, &ai);

	auto t = Clock::now();
	while (!world.is_over()) {
		while (glGetError() != GL_NO_ERROR);
		
		glfwPollEvents();

		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		const bool pause_for_tutorial = tutorial.should_pause();
		const bool pause_for_inventory = inventory.is_inventory_open();
		if (!pause_for_tutorial && !pause_for_inventory) {
			world.step(elapsed_ms);
			ai.step(elapsed_ms);
			physics.step(elapsed_ms);
			world.handle_collisions();
		}
		
		inventory.update(elapsed_ms);
		tutorial.update(elapsed_ms);
		
		renderer.draw();
		
		stats.render();
		inventory.render();
		tutorial.render();
		
		glfwSwapBuffers(window);
	}

	return EXIT_SUCCESS;
}
