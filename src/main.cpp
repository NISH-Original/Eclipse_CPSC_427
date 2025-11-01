
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

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

using Clock = std::chrono::high_resolution_clock;

// Entry point
int main()
{
#ifdef _WIN32
	char exe_path_buf[MAX_PATH];
	if (GetModuleFileNameA(nullptr, exe_path_buf, MAX_PATH)) {
		std::string exe_path = exe_path_buf;
		size_t last_slash = exe_path.find_last_of("\\/");
		if (last_slash != std::string::npos) {
			std::string exe_dir = exe_path.substr(0, last_slash);
			_chdir(exe_dir.c_str());
		}
	}
#else
	char exe_path_buf[PATH_MAX];
	ssize_t count = readlink("/proc/self/exe", exe_path_buf, PATH_MAX);
	if (count != -1) {
		exe_path_buf[count] = '\0';
		std::string exe_path = exe_path_buf;
		size_t last_slash = exe_path.find_last_of("/");
		if (last_slash != std::string::npos) {
			std::string exe_dir = exe_path.substr(0, last_slash);
			chdir(exe_dir.c_str());
		}
	}
#endif
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
	
	world.init(&renderer, &inventory, &stats, &objectives, &minimap, &currency, &ai);

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
