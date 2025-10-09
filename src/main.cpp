
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>

// internal
#include "physics_system.hpp"
#include "render_system.hpp"
#include "world_system.hpp"
#include "inventory_system.hpp"

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
	// Global systems
	WorldSystem world;
	RenderSystem renderer;
	PhysicsSystem physics;
	InventorySystem inventory;

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
	world.init(&renderer, &inventory);

	// variable timestep loop
	auto t = Clock::now();
	while (!world.is_over()) {
		// Clear any OpenGL errors from previous frame (especially from UI rendering)
		while (glGetError() != GL_NO_ERROR);
		
		// Processes system messages, if this wasn't present the window would become unresponsive
		glfwPollEvents();

		// Calculating elapsed times in milliseconds from the previous iteration
		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		world.step(elapsed_ms);
		physics.step(elapsed_ms);
		world.handle_collisions();
		
		// Update and render inventory
		inventory.update(elapsed_ms);

		renderer.draw();
		
		// Render inventory UI on top
		inventory.render();
		
		// Swap buffers to display the UI
		glfwSwapBuffers(window);
	}

	return EXIT_SUCCESS;
}
