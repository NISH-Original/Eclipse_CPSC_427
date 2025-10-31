
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
#include "ai_system.hpp"
#include "audio_system.hpp"

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
	AISystem ai;
	AudioSystem audio;

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

	// Initialize audio system and load sounds
	audio.init();
	audio.load("gunshot", "data/audio/gunshot.wav");
	audio.load("ambient", "data/audio/ambient.wav");
	audio.load("impact-enemy", "data/audio/impact-enemy.wav");
	audio.load("impact-tree", "data/audio/impact-tree.wav");

	// Play ambient music on loop
	audio.play("ambient", true);

	world.init(&renderer, &inventory, &stats, &objectives, &minimap, &currency, &ai, &audio);

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
		ai.step(elapsed_ms);
		physics.step(elapsed_ms);
		world.handle_collisions();
		
		// Update and render inventory
		inventory.update(elapsed_ms);
		
		renderer.draw();

		// Save OpenGL State before UI rendering
		GLint saved_vao, saved_program, saved_framebuffer;
		GLint saved_array_buffer, saved_element_buffer;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &saved_vao);
		glGetIntegerv(GL_CURRENT_PROGRAM, &saved_program);
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &saved_framebuffer);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &saved_array_buffer);
		glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &saved_element_buffer);

		stats.render();
		inventory.render();

		// The UI rendering was corrupting the OpenGL state
		// So restore OpenGL state after UI rendering
		// This is a bit hacky, but it works
		glBindVertexArray(saved_vao);
		glUseProgram(saved_program);
		glBindFramebuffer(GL_FRAMEBUFFER, saved_framebuffer);
		glBindBuffer(GL_ARRAY_BUFFER, saved_array_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, saved_element_buffer);

		glfwSwapBuffers(window);
	}

	return EXIT_SUCCESS;
}
