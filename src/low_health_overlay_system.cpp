// internal
#include "low_health_overlay_system.hpp"
#include "health_system.hpp"
#include <cmath>
#include <array>

LowHealthOverlaySystem::LowHealthOverlaySystem()
{
}

void LowHealthOverlaySystem::init(
	GLFWwindow* window_arg,
	const std::array<GLuint, texture_count>& texture_gl_handles_arg,
	const std::array<GLuint, effect_count>& effects_arg,
	const std::array<GLuint, geometry_count>& vertex_buffers_arg,
	const std::array<GLuint, geometry_count>& index_buffers_arg,
	HealthSystem* health_system_arg
)
{
	this->window = window_arg;
	this->texture_gl_handles = &texture_gl_handles_arg;
	this->effects = &effects_arg;
	this->vertex_buffers = &vertex_buffers_arg;
	this->index_buffers = &index_buffers_arg;
	this->health_system = health_system_arg;
}

void LowHealthOverlaySystem::set_health_system(HealthSystem* health_system_arg)
{
	this->health_system = health_system_arg;
}

void LowHealthOverlaySystem::render(float elapsed_ms)
{
	bool is_below_20_percent = false;
	float health_percent = 100.0f;
	
	if (health_system && health_system->has_player()) {
		Entity player_entity = health_system->get_player_entity();
		
		health_percent = health_system->get_health_percent(player_entity);
		
		is_below_20_percent = health_percent <= 20.0f;
		bool is_below_10_percent = health_percent <= 10.0f;
		
		// Handle entering low health (damage taken)
		if (is_below_20_percent && !was_below_20_percent) {
			low_health_overlay_active = true;
			low_health_animation_timer = 0.0f;
			first_animation_complete = false;
			phase2_start_scale = PHASE1_END_SCALE; // Reset to default
			was_below_10_percent = is_below_10_percent;
			is_healing_animation = false;
		}
		
		// Handle dropping below 10% (more damage)
		if (is_below_10_percent && !was_below_10_percent && was_below_20_percent) {
			if (!first_animation_complete) {
				float phase1_progress = glm::clamp(low_health_animation_timer / LOW_HEALTH_ANIMATION_DURATION, 0.0f, 1.0f);
				float phase1_range = PHASE1_START_SCALE - PHASE1_END_SCALE;
				phase2_start_scale = PHASE1_START_SCALE - (phase1_range * phase1_progress); // Current scale from phase 1
			} else {
				phase2_start_scale = PHASE1_END_SCALE;
			}
			low_health_animation_timer = 0.0f;
			is_healing_animation = false;
		}
		
		// Handle healing above 10% (from <=10% to >10%)
		if (!is_below_10_percent && was_below_10_percent) {
			is_healing_animation = true;
			phase2_start_scale = PHASE2_END_SCALE;
			low_health_animation_timer = 0.0f;
		}
		
		// Handle healing above 20% (from <=20% to >20%)
		if (!is_below_20_percent && was_below_20_percent) {
			is_healing_animation = true;
			phase2_start_scale = PHASE1_END_SCALE;
			low_health_animation_timer = 0.0f;
		}
		
		// Update animation timer
		if (is_below_20_percent || is_healing_animation) {
			low_health_overlay_active = true;
			
			if (low_health_animation_timer < LOW_HEALTH_ANIMATION_DURATION) {
				low_health_animation_timer += elapsed_ms / 1000.0f; // Convert to seconds
				
				if (low_health_animation_timer > LOW_HEALTH_ANIMATION_DURATION) {
					low_health_animation_timer = LOW_HEALTH_ANIMATION_DURATION;
					if (!is_below_10_percent && is_below_20_percent) {
						first_animation_complete = true;
					}
					if (is_healing_animation && !is_below_20_percent) {
						low_health_overlay_active = false;
						is_healing_animation = false;
					}
				}
			} else if (!is_below_10_percent && is_below_20_percent) {
				first_animation_complete = true;
			}
		}
		
		was_below_20_percent = is_below_20_percent;
		was_below_10_percent = is_below_10_percent;
	}
	
	if (!low_health_overlay_active) {
		return;
	}
	
	float scale;
	float animation_progress = low_health_animation_timer / LOW_HEALTH_ANIMATION_DURATION;
	animation_progress = glm::clamp(animation_progress, 0.0f, 1.0f);
	
	if (is_healing_animation) {
		float target_scale = (phase2_start_scale == PHASE2_END_SCALE) ? PHASE1_END_SCALE : PHASE1_START_SCALE;
		scale = phase2_start_scale + ((target_scale - phase2_start_scale) * animation_progress);
		
		if (scale >= PHASE1_START_SCALE) {
			low_health_overlay_active = false;
			is_healing_animation = false;
			return;
		}
	} else if (health_percent <= 10.0f) {
		scale = phase2_start_scale - ((phase2_start_scale - PHASE2_END_SCALE) * animation_progress);
	} else {
		float phase1_range = PHASE1_START_SCALE - PHASE1_END_SCALE;
		scale = PHASE1_START_SCALE - (phase1_range * animation_progress);
	}
	
	drawOverlay(scale);
}

void LowHealthOverlaySystem::drawOverlay(float scale)
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	gl_has_errors();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	gl_has_errors();
	
	const GLuint program = effects->at((GLuint)EFFECT_ASSET_ID::TEXTURED);
	glUseProgram(program);
	gl_has_errors();
	
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers->at((GLuint)GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers->at((GLuint)GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD));
	gl_has_errors();
	
	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
						  sizeof(TexturedVertex), (void *)0);
	gl_has_errors();
	
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
						  (void *)sizeof(vec3));
	gl_has_errors();
	
	GLint total_row_uloc = glGetUniformLocation(program, "total_row");
	if (total_row_uloc >= 0) glUniform1i(total_row_uloc, 1);
	
	GLint curr_row_uloc = glGetUniformLocation(program, "curr_row");
	if (curr_row_uloc >= 0) glUniform1i(curr_row_uloc, 0);
	
	GLint total_frame_uloc = glGetUniformLocation(program, "total_frame");
	if (total_frame_uloc >= 0) glUniform1i(total_frame_uloc, 1);
	
	GLint curr_frame_uloc = glGetUniformLocation(program, "curr_frame");
	if (curr_frame_uloc >= 0) glUniform1i(curr_frame_uloc, 0);
	
	GLint should_flip_uloc = glGetUniformLocation(program, "should_flip");
	if (should_flip_uloc >= 0) glUniform1i(should_flip_uloc, 0);
	
	GLint is_hurt_uloc = glGetUniformLocation(program, "is_hurt");
	if (is_hurt_uloc >= 0) glUniform1i(is_hurt_uloc, 0);
	
	GLint alpha_mod_uloc = glGetUniformLocation(program, "alpha_mod");
	if (alpha_mod_uloc >= 0) glUniform1f(alpha_mod_uloc, 1.0f);
	
	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles->at((GLuint)TEXTURE_ASSET_ID::LOW_HEALTH_BLOOD);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();
	
	GLint viewport_loc = glGetUniformLocation(program, "viewport_size");
	if (viewport_loc >= 0) glUniform2f(viewport_loc, (float)w, (float)h);
	
	GLint ambient_loc = glGetUniformLocation(program, "ambient_light");
	if (ambient_loc >= 0) glUniform1f(ambient_loc, 1.0f);
	
	GLint camera_offset_loc = glGetUniformLocation(program, "camera_offset");
	if (camera_offset_loc >= 0) glUniform2f(camera_offset_loc, 0.0f, 0.0f);
	
	mat3 transform = { {scale,0,0}, {0,scale,0}, {0,0,1} };
	mat3 projection = { {1,0,0}, {0,1,0}, {0,0,1} };
	
	GLint transform_loc = glGetUniformLocation(program, "transform");
	if (transform_loc >= 0) glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&transform);
	
	GLint projection_loc = glGetUniformLocation(program, "projection");
	if (projection_loc >= 0) glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float*)&projection);
	
	gl_has_errors();
	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
	
	glDisableVertexAttribArray(in_position_loc);
	glDisableVertexAttribArray(in_texcoord_loc);
	gl_has_errors();
}

