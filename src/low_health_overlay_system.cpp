// internal
#include "low_health_overlay_system.hpp"
#include <cmath>

LowHealthOverlaySystem::LowHealthOverlaySystem()
{
}

void LowHealthOverlaySystem::init(
	GLFWwindow* window,
	const std::array<GLuint, texture_count>& texture_gl_handles,
	const std::array<GLuint, effect_count>& effects,
	const std::array<GLuint, geometry_count>& vertex_buffers,
	const std::array<GLuint, geometry_count>& index_buffers
)
{
	this->window = window;
	this->texture_gl_handles = &texture_gl_handles;
	this->effects = &effects;
	this->vertex_buffers = &vertex_buffers;
	this->index_buffers = &index_buffers;
}

void LowHealthOverlaySystem::render(float elapsed_ms)
{
	bool is_below_20_percent = false;
	float health_percent = 100.0f;
	
	if (registry.players.entities.size() > 0) {
		Entity player_entity = registry.players.entities[0];
		if (registry.players.has(player_entity)) {
			Player& player = registry.players.get(player_entity);
			
			health_percent = (float)player.health / (float)player.max_health * 100.0f;
			
			is_below_20_percent = health_percent <= 20.0f;
			
			bool is_below_10_percent = health_percent <= 10.0f;
			
			if (is_below_20_percent && !was_below_20_percent) {
				low_health_overlay_active = true;
				low_health_animation_timer = 0.0f;
				first_animation_complete = false;
				phase2_start_scale = PHASE1_END_SCALE; // Reset to default
				was_below_10_percent = is_below_10_percent;
			}
			
			if (is_below_10_percent && !was_below_10_percent && was_below_20_percent) {
				if (!first_animation_complete) {
					float phase1_progress = glm::clamp(low_health_animation_timer / LOW_HEALTH_ANIMATION_DURATION, 0.0f, 1.0f);
					float phase1_range = PHASE1_START_SCALE - PHASE1_END_SCALE;
					phase2_start_scale = PHASE1_START_SCALE - (phase1_range * phase1_progress); // Current scale from phase 1
				} else {
					phase2_start_scale = PHASE1_END_SCALE;
				}
				low_health_animation_timer = 0.0f;
			}
			
			if (is_below_20_percent) {
				low_health_overlay_active = true;
				
				if (low_health_animation_timer < LOW_HEALTH_ANIMATION_DURATION) {
					low_health_animation_timer += elapsed_ms / 1000.0f; // Convert to seconds
					
					if (low_health_animation_timer > LOW_HEALTH_ANIMATION_DURATION) {
						low_health_animation_timer = LOW_HEALTH_ANIMATION_DURATION;
						if (!is_below_10_percent) {
							first_animation_complete = true;
						}
					}
				} else if (!is_below_10_percent) {
					first_animation_complete = true;
				}
			} else {
				low_health_overlay_active = false;
				low_health_animation_timer = 0.0f;
				first_animation_complete = false;
				phase2_start_scale = PHASE1_END_SCALE; // Reset to default
				was_below_10_percent = false;
			}
			
			was_below_20_percent = is_below_20_percent;
			was_below_10_percent = is_below_10_percent;
		}
	}
	
	if (!low_health_overlay_active) {
		return;
	}
	
	float scale;
	float animation_progress = low_health_animation_timer / LOW_HEALTH_ANIMATION_DURATION;
	animation_progress = glm::clamp(animation_progress, 0.0f, 1.0f);
	
	if (health_percent <= 10.0f) {
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
	
	const GLuint program = (*effects)[(GLuint)EFFECT_ASSET_ID::TEXTURED];
	glUseProgram(program);
	gl_has_errors();
	
	glBindBuffer(GL_ARRAY_BUFFER, (*vertex_buffers)[(GLuint)GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (*index_buffers)[(GLuint)GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD]);
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
	GLuint texture_id = (*texture_gl_handles)[(GLuint)TEXTURE_ASSET_ID::LOW_HEALTH_BLOOD];
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

