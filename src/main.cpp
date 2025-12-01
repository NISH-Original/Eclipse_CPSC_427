
#define GL3W_IMPLEMENTATION
#include <gl3w.h>

// stlib
#include <chrono>
#include <thread>
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
#include "start_menu_system.hpp"
#include "ai_system.hpp"
#include "pathfinding_system.hpp"
#include "steering_system.hpp"
#include "audio_system.hpp"
#include "save_system.hpp"

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
	StartMenuSystem start_menu;
	TutorialSystem tutorial;
	AISystem ai;
	PathfindingSystem pathfinding;
	SteeringSystem steering;
	AudioSystem audio;
	SaveSystem save_system;

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
	inventory.set_audio_system(&audio);
	ai.init(&renderer, &audio);

	stats.init(inventory.get_context());
	objectives.init(inventory.get_context());
	minimap.init(inventory.get_context());
	currency.init(inventory.get_context());
	menu_icons.init(inventory.get_context(), &audio);
	tutorial.init(inventory.get_context());
	start_menu.init(inventory.get_context(), &audio);


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
	audio.load("rifle_gunshot", "data/audio/rifle_gunshot.wav");
	audio.load("ambient", "data/audio/ambient.wav");
	audio.load("impact-enemy", "data/audio/impact-enemy.wav");
	audio.load("impact-tree", "data/audio/impact-tree.wav");
	audio.load("reload", "data/audio/reload.wav");
	audio.load("dash", "data/audio/dash.wav");
	audio.load("hurt", "data/audio/hurt.wav");
	audio.load("game_lose", "data/audio/game_lose_dramatic.wav");
	audio.load("heart_beat", "data/audio/heart_beat.wav");
	audio.load("game_start", "data/audio/game_start.wav");
	audio.load("xylarite_collect", "data/audio/xylarite_collect.wav");
	audio.load("xylarite_spend", "data/audio/xylarite_spend.wav");
	audio.load("heal_inhale", "data/audio/heal_inhale.wav");

	// Play ambient music on loop
	audio.play("ambient", true);

	world.init(&renderer, &inventory, &stats, &objectives, &minimap, &currency, &menu_icons, &tutorial, &start_menu, &ai, &audio, &save_system);

	// Initialize FPS history
	float fps_history[60] = {0};
	int fps_index = 0;
	
	// Track previous pause state to detect transitions
	bool was_paused = false;
	
	// target 60 FPS
	const float target_frame_time_ms = 1000.0f / 60.0f;

	auto t = Clock::now();
	while (!world.is_over()) {
		while (glGetError() != GL_NO_ERROR);
		
		glfwPollEvents();

		auto now = Clock::now();
		float elapsed_ms =
			(float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
		
		// limit FPS to 60 by sleeping if frame completed too quickly
		if (elapsed_ms < target_frame_time_ms) {
			float sleep_time_ms = target_frame_time_ms - elapsed_ms - 0.5f; // 0.5ms for overhead
			if (sleep_time_ms > 0.5f) {
				std::this_thread::sleep_for(std::chrono::microseconds((int)(sleep_time_ms * 1000)));
			}
			// busy-wait for the remaining time
			while (true) {
				now = Clock::now();
				elapsed_ms = (float)(std::chrono::duration_cast<std::chrono::microseconds>(now - t)).count() / 1000;
				if (elapsed_ms >= target_frame_time_ms) {
					break;
				}
			}
		}
		
		t = now;

	const bool pause_for_tutorial = tutorial.should_pause();
	const bool pause_for_inventory = inventory.is_inventory_open();
	const bool pause_for_start_menu = world.is_start_menu_active();
	const bool pause_for_level_transition = world.is_level_transition_active();
	const bool is_paused = pause_for_tutorial || pause_for_inventory || pause_for_start_menu || pause_for_level_transition;
	
	// Restore cursor when game resumes from pause
	if (was_paused && !is_paused) {
		world.update_crosshair_cursor();
	}
	was_paused = is_paused;

		// Update FPS display
#ifdef HAVE_RMLUI
		if (fps_document) {
			// Hide FPS display when start menu is active
			Rml::Element* fps_display = fps_document->GetElementById("fps_display");
			if (fps_display) {
				fps_display->SetProperty("display", pause_for_start_menu ? "none" : "block");
			}
			
			if (!pause_for_start_menu) {
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
		}
#endif

	if (!pause_for_tutorial && !pause_for_inventory && !pause_for_start_menu && !pause_for_level_transition) {
		world.step(elapsed_ms);
		pathfinding.step(elapsed_ms);
		steering.step(elapsed_ms);
		ai.step(elapsed_ms);
		physics.step(elapsed_ms);
		world.sync_feet_to_player();
		world.handle_collisions();
	} else {
		world.update_paused(elapsed_ms);
	}
	
		inventory.update(elapsed_ms);
		tutorial.update(elapsed_ms);
		
		stats.set_ammo_counter_opacity(is_paused ? 0.0f : 1.0f);
		
		renderer.draw(elapsed_ms, is_paused);

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

	// Cleanup systems before exit to prevent segmentation fault
	audio.cleanup();
	
#ifdef HAVE_RMLUI
	// Shutdown RmlUI after all UI systems are destroyed to prevent race condition
	Rml::Shutdown();
#endif

	return EXIT_SUCCESS;
}
