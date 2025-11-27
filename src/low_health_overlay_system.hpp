#pragma once

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"

// System responsible for rendering the low health blood overlay
class LowHealthOverlaySystem
{
public:
	LowHealthOverlaySystem();
	
	// Initialize the system with required OpenGL resources
	void init(
		GLFWwindow* window,
		const std::array<GLuint, texture_count>& texture_gl_handles,
		const std::array<GLuint, effect_count>& effects,
		const std::array<GLuint, geometry_count>& vertex_buffers,
		const std::array<GLuint, geometry_count>& index_buffers
	);
	
	// Update and render the overlay
	void render(float elapsed_ms);

private:
	// Animation state
	bool low_health_overlay_active = false;
	float low_health_animation_timer = 0.0f;
	const float LOW_HEALTH_ANIMATION_DURATION = 0.8f; // Animation duration in seconds
	const float PHASE1_START_SCALE = 1.5f; // Starting scale for phase 1
	const float PHASE1_END_SCALE = 1.2f; // Ending scale for phase 1 (10-20% health)
	const float PHASE2_END_SCALE = 1.1f; // Ending scale for phase 2 (below 10% health)
	bool was_below_20_percent = false;
	bool was_below_10_percent = false;
	bool first_animation_complete = false; // Track if 1.5x to 1.2x animation is complete
	float phase2_start_scale = 1.2f; // Starting scale for phase 2
	
	// OpenGL resources (references to avoid copying)
	GLFWwindow* window = nullptr;
	const std::array<GLuint, texture_count>* texture_gl_handles = nullptr;
	const std::array<GLuint, effect_count>* effects = nullptr;
	const std::array<GLuint, geometry_count>* vertex_buffers = nullptr;
	const std::array<GLuint, geometry_count>* index_buffers = nullptr;
	
	// Internal rendering function
	void drawOverlay(float scale);
};

