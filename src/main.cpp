
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
#include "tutorial_system.hpp"
#include "ai_system.hpp"
#include "pathfinding_system.hpp"
#include "steering_system.hpp"
#include "audio_system.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

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
	TutorialSystem tutorial;
	AISystem ai;
	PathfindingSystem pathfinding;
	SteeringSystem steering;
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
	tutorial.init(inventory.get_context());

	// Initialize FPS display
#ifdef HAVE_RMLUI
	Rml::ElementDocument* fps_document = inventory.get_context()->LoadDocument("ui/fps.rml");
	if (fps_document) {
		fps_document->Show();
	} 
#endif

	// Initialize audio system and load sounds
	audio.init();
	audio.load("gunshot", "data/audio/gunshot.wav");
	audio.load("shotgun_gunshot", "data/audio/shotgun_gunshot.wav");
	audio.load("ambient", "data/audio/ambient.wav");
	audio.load("impact-enemy", "data/audio/impact-enemy.wav");
	audio.load("impact-tree", "data/audio/impact-tree.wav");

	// Play ambient music on loop
	audio.play("ambient", true);

	world.init(&renderer, &inventory, &stats, &objectives, &minimap, &currency, &tutorial, &ai, &audio);

	// Initialize FPS history
	float fps_history[60] = {0};
	int fps_index = 0;

	auto t = Clock::now();
	while (!world.is_over()) {
		while (glGetError() != GL_NO_ERROR);
		
		glfwPollEvents();

		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		t = now;

		// Update FPS display
#ifdef HAVE_RMLUI
		if (fps_document) {
			// Calculate current FPS
			float current_fps = elapsed_ms > 0 ? 1000.f / elapsed_ms : 0.f;
			// Add current FPS to history
			fps_history[fps_index] = current_fps;
			fps_index = (fps_index + 1) % 60;

			// Calculate average FPS
			float avg_fps = 0.f;
			for (int i = 0; i < 60; i++) {
				avg_fps += fps_history[i];
			}
			avg_fps /= 60.f;

			// Update FPS display
			Rml::Element* fps_value = fps_document->GetElementById("fps_value");
			if (fps_value) {
				char fps_str[32];
				snprintf(fps_str, sizeof(fps_str), "%.0f", avg_fps);
				fps_value->SetInnerRML(fps_str);
			}
		}
#endif

	const bool pause_for_tutorial = tutorial.should_pause();
	const bool pause_for_inventory = inventory.is_inventory_open();
	if (!pause_for_tutorial && !pause_for_inventory) {
		world.step(elapsed_ms);
		pathfinding.step(elapsed_ms);
		steering.step(elapsed_ms);
		ai.step(elapsed_ms);
		physics.step(elapsed_ms);
		world.handle_collisions();
	}
		
		inventory.update(elapsed_ms);
		tutorial.update(elapsed_ms);
		
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
		tutorial.render();

		glfwSwapBuffers(window);
	}

	return EXIT_SUCCESS;
}
