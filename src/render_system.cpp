// internal
#include "render_system.hpp"
#include <SDL.h>
#include <iostream>

#include "tiny_ecs_registry.hpp"

static GLuint g_debug_line_vbo = 0;

void RenderSystem::drawTexturedMesh(Entity entity,
									const mat3 &projection)
{
	Motion &motion = registry.motions.get(entity);
	// Transformation code, see Rendering and Transformation in the template
	// specification for more info Incrementally updates transformation matrix,
	// thus ORDER IS IMPORTANT
	Transform transform;
	transform.translate(motion.position);
	transform.rotate(motion.angle);
	transform.scale(motion.scale);
	// of transformations

	assert(registry.renderRequests.has(entity));
	const RenderRequest &render_request = registry.renderRequests.get(entity);

	const GLuint used_effect_enum = (GLuint)render_request.used_effect;
	assert(used_effect_enum != (GLuint)EFFECT_ASSET_ID::EFFECT_COUNT);
	const GLuint program = (GLuint)effects[used_effect_enum];

	// Setting shaders
	glUseProgram(program);
	gl_has_errors();

	assert(render_request.used_geometry != GEOMETRY_BUFFER_ID::GEOMETRY_COUNT);
	const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
	const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

	// Setting vertex and index buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	// Input data location as in the vertex buffer
	if (render_request.used_effect == EFFECT_ASSET_ID::TEXTURED)
	{
		Sprite& sprite = registry.sprites.get(entity);

		GLint total_frame_uloc = glGetUniformLocation(program, "total_frame");
		assert(total_frame_uloc >= 0);
		glUniform1i(total_frame_uloc, sprite.total_frame);

		GLint curr_frame_uloc = glGetUniformLocation(program, "curr_frame");
		assert(curr_frame_uloc >= 0);
		glUniform1i(curr_frame_uloc, sprite.curr_frame);

		GLint should_flip_uloc = glGetUniformLocation(program, "should_flip");
		assert(should_flip_uloc >= 0);
		glUniform1i(should_flip_uloc, sprite.should_flip);

		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
		gl_has_errors();
		assert(in_texcoord_loc >= 0);

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
							  sizeof(TexturedVertex), (void *)0);
		gl_has_errors();

		glEnableVertexAttribArray(in_texcoord_loc);
		glVertexAttribPointer(
			in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
			(void *)sizeof(
				vec3)); // note the stride to skip the preceeding vertex position

		// Enabling and binding texture to slot 0
		glActiveTexture(GL_TEXTURE0);
		gl_has_errors();

		assert(registry.renderRequests.has(entity));
		GLuint texture_id =
			texture_gl_handles[(GLuint)registry.renderRequests.get(entity).used_texture];

		glBindTexture(GL_TEXTURE_2D, texture_id);
		gl_has_errors();
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::SALMON || render_request.used_effect == EFFECT_ASSET_ID::COLOURED)
	{
		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_color_loc = glGetAttribLocation(program, "in_color");
		gl_has_errors();

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
							  sizeof(ColoredVertex), (void *)0);
		gl_has_errors();

		glEnableVertexAttribArray(in_color_loc);
			glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
							  	sizeof(ColoredVertex), (void *)sizeof(vec3));
		gl_has_errors();

		if (render_request.used_effect == EFFECT_ASSET_ID::SALMON)
		{
			// Light up?
			GLint light_up_uloc = glGetUniformLocation(program, "light_up");
			assert(light_up_uloc >= 0);

			// similar to the glUniform1f call below. The 1f or 1i specified the type, here a single int.
			gl_has_errors();
		}
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::LIGHT)
	{
		// Light shader handling is done below
	}
	else
	{
		assert(false && "Type of render request not supported");
	}

	if (render_request.used_effect == EFFECT_ASSET_ID::LIGHT)
	{	
		
		GLint global_ambient_brightness_loc = glGetUniformLocation(program, "global_ambient_brightness");
		if (global_ambient_brightness_loc >= 0) glUniform1f(global_ambient_brightness_loc, global_ambient_brightness);
		gl_has_errors();
		
		// Put the occlusion mask in texture slot 1
		glActiveTexture(GL_TEXTURE1);
		gl_has_errors();
		if (occlusion_texture != 0) {
			glBindTexture(GL_TEXTURE_2D, occlusion_texture);
		}
		gl_has_errors();
		GLint occlusion_texture_loc = glGetUniformLocation(program, "occlusion_texture");
		gl_has_errors();
		if (occlusion_texture_loc >= 0) glUniform1i(occlusion_texture_loc, 1);
		gl_has_errors();
		
		// Pass screen size for texture coordinate conversion
		GLint screen_size_loc = glGetUniformLocation(program, "screen_size");
		gl_has_errors();
		if (screen_size_loc >= 0) {
			vec2 screen_size = {(float)window_width_px, (float)window_height_px};
			glUniform2fv(screen_size_loc, 1, (float*)&screen_size);
		}
		gl_has_errors();

		GLint in_position_loc = glGetAttribLocation(program, "in_position");
		GLint in_color_loc = glGetAttribLocation(program, "in_color");
		gl_has_errors();

		glEnableVertexAttribArray(in_position_loc);
		glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
							sizeof(ColoredVertex), (void *)0);
		gl_has_errors();

		glEnableVertexAttribArray(in_color_loc);
		glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
							sizeof(ColoredVertex), (void *)sizeof(vec3));
		gl_has_errors();

		Entity flashlight_entity;
		Light light_data;
		bool found_flashlight = false;
		
		if (!registry.lights.entities.empty()) {
			flashlight_entity = registry.lights.entities[0];
			light_data = registry.lights.get(flashlight_entity);
			found_flashlight = true;
		}
		
		if (found_flashlight && registry.motions.has(flashlight_entity)) {
			Motion& flashlight_motion = registry.motions.get(flashlight_entity);
			
			GLint player_pos_loc = glGetUniformLocation(program, "player_position");
			GLint player_dir_loc = glGetUniformLocation(program, "player_direction");
			
			vec2 light_position;
			vec2 direction;
			
			// make the light follow the entity
			if (registry.motions.has(light_data.follow_target)) {
				Motion& target_motion = registry.motions.get(light_data.follow_target);
				
				// do we use the angle of the entity?
				if (light_data.use_target_angle) {
					direction = { cos(target_motion.angle), sin(target_motion.angle) };
				} else {
					direction = { cos(flashlight_motion.angle), sin(flashlight_motion.angle) };
				}
				
				float cos_angle = cos(target_motion.angle);
				float sin_angle = sin(target_motion.angle);
				vec2 rotated_offset = {
					light_data.offset.x * cos_angle - light_data.offset.y * sin_angle,
					light_data.offset.x * sin_angle + light_data.offset.y * cos_angle
				};

				light_position = target_motion.position + rotated_offset;
			} else {
				light_position = flashlight_motion.position;
				direction = { cos(flashlight_motion.angle), sin(flashlight_motion.angle) };
			}
			
			if (player_pos_loc >= 0) glUniform2fv(player_pos_loc, 1, (float*)&light_position);
			if (player_dir_loc >= 0) glUniform2fv(player_dir_loc, 1, (float*)&direction);
			
			GLint cone_angle_loc = glGetUniformLocation(program, "light_cone_angle");
			GLint brightness_loc = glGetUniformLocation(program, "light_brightness");
			GLint falloff_loc = glGetUniformLocation(program, "light_brightness_falloff");
			GLint range_loc = glGetUniformLocation(program, "light_range");
			GLint inner_cone_loc = glGetUniformLocation(program, "light_inner_cone_angle");
			GLint color_loc = glGetUniformLocation(program, "light_color");
			
			if (cone_angle_loc >= 0) glUniform1f(cone_angle_loc, light_data.cone_angle);
			if (brightness_loc >= 0) glUniform1f(brightness_loc, light_data.brightness);
			if (falloff_loc >= 0) glUniform1f(falloff_loc, light_data.falloff);
			if (range_loc >= 0) glUniform1f(range_loc, light_data.range);
			if (color_loc >= 0) glUniform3fv(color_loc, 1, (float*)&light_data.light_color);
			if (inner_cone_loc >= 0) glUniform1f(inner_cone_loc, light_data.inner_cone_angle);
		}
	}

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = registry.colors.has(entity) ? registry.colors.get(entity) : vec3(1);
	glUniform3fv(color_uloc, 1, (float *)&color);
	gl_has_errors();

	// Get number of indices from index buffer, which has elements uint16_t
	GLint size = 0;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
	gl_has_errors();

	GLsizei num_indices = size / sizeof(uint16_t);
	// GLsizei num_triangles = num_indices / 3;

	GLint currProgram;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
	// Setting uniform values to the currently bound program
	GLuint transform_loc = glGetUniformLocation(currProgram, "transform");
	glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
	GLuint projection_loc = glGetUniformLocation(currProgram, "projection");
	glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection);
	gl_has_errors();
	// Drawing of num_indices/3 triangles specified in the index buffer
	glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

// draw the intermediate texture to the screen, with some distortion to simulate
// water
void RenderSystem::drawToScreen()
{
	// Setting shaders
	// get the water texture, sprite mesh, and program
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::WATER]);
	gl_has_errors();
	// Clearing backbuffer
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();
	// Enabling alpha channel for textures
	glDisable(GL_BLEND);
	// glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	// Draw the screen texture on the quad geometry
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]); // Note, GL_ELEMENT_ARRAY_BUFFER associates
																	 // indices to the bound GL_ARRAY_BUFFER
	gl_has_errors();
	const GLuint water_program = effects[(GLuint)EFFECT_ASSET_ID::WATER];
	// Set clock
	GLuint time_uloc = glGetUniformLocation(water_program, "time");
	GLuint dead_timer_uloc = glGetUniformLocation(water_program, "darken_screen_factor");
	glUniform1f(time_uloc, (float)(glfwGetTime() * 10.0f));
	ScreenState &screen = registry.screenStates.get(screen_state_entity);
	glUniform1f(dead_timer_uloc, screen.darken_screen_factor);
	gl_has_errors();
	// Set the vertex position and vertex texture coordinates (both stored in the
	// same VBO)
	GLint in_position_loc = glGetAttribLocation(water_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	gl_has_errors();

	// Bind our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);

	// Debug visualization: show occlusion mask if enabled
	if (debugging.show_occlusion_mask) {
		glBindTexture(GL_TEXTURE_2D, occlusion_texture);
	} else {
		glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	}
	gl_has_errors();
	// Draw
	glDrawElements(
		GL_TRIANGLES, 3, GL_UNSIGNED_SHORT,
		nullptr); // one triangle = 3 vertices; nullptr indicates that there is
				  // no offset from the bound index buffer
	gl_has_errors();
}

// Render all occluders to the occlusion texture as a black/white mask
void RenderSystem::renderOcclusionMask()
{
	// Bind occlusion framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, occlusion_frame_buffer);
	gl_has_errors();
	
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glViewport(0, 0, w, h);
	
	// Clear to black (no occlusion)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	gl_has_errors();
	
	// Disable blending for sharp mask
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	
	// bind VAO
	glBindVertexArray(vao);

	mat3 projection_2D = createProjectionMatrix();
	
	// Render all entities with Occluder component as white
	for (Entity entity : registry.occluders.entities)
	{
		if (!registry.motions.has(entity) || !registry.renderRequests.has(entity))
			continue;
			
		Motion &motion = registry.motions.get(entity);
		
		// Transformation code, see Rendering and Transformation in the template
		// specification for more info Incrementally updates transformation matrix,
		// thus ORDER IS IMPORTANT
		Transform transform;
		transform.translate(motion.position);
		transform.rotate(motion.angle);
		transform.scale(motion.scale);

		const RenderRequest &render_request = registry.renderRequests.get(entity);

		const GLuint used_effect_enum = (GLuint) EFFECT_ASSET_ID::COLOURED;
		const GLuint program = (GLuint)effects[used_effect_enum];

		// Setting shaders
		glUseProgram(program);
		gl_has_errors();

		const GLuint vbo = vertex_buffers[(GLuint)render_request.used_geometry];
		const GLuint ibo = index_buffers[(GLuint)render_request.used_geometry];

		// Setting vertex and index buffers
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		gl_has_errors();

		// Input data location as in the vertex buffer
		if (render_request.used_geometry == GEOMETRY_BUFFER_ID::SPRITE) {
			// Textured geometry
			GLint in_position_loc = glGetAttribLocation(program, "in_position");
			if (in_position_loc >= 0) {
				glEnableVertexAttribArray(in_position_loc);
				glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
									  sizeof(TexturedVertex), (void *)0);
			}
			gl_has_errors();
		} else {
			// Colored geometry
			GLint in_position_loc = glGetAttribLocation(program, "in_position");
			GLint in_color_loc = glGetAttribLocation(program, "in_color");
			gl_has_errors();

			if (in_position_loc >= 0) {
				glEnableVertexAttribArray(in_position_loc);
				glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
									  sizeof(ColoredVertex), (void *)0);
			}
			gl_has_errors();

			if (in_color_loc >= 0) {
				glEnableVertexAttribArray(in_color_loc);
				glVertexAttribPointer(in_color_loc, 3, GL_FLOAT, GL_FALSE,
									  sizeof(ColoredVertex), (void *)sizeof(vec3));
			}
			gl_has_errors();
		}

		// Getting uniform locations for glUniform* calls
		GLint color_uloc = glGetUniformLocation(program, "color");
		const vec3 white_color = vec3(1); // white for occlusion mask
		if (color_uloc >= 0) {
			glUniform3fv(color_uloc, 1, (float *)&white_color);
		}
		gl_has_errors();

		// Get number of indices from index buffer, which has elements uint16_t
		GLint size = 0;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);
		gl_has_errors();

		GLsizei num_indices = size / sizeof(uint16_t);
		// GLsizei num_triangles = num_indices / 3;

		GLint currProgram;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currProgram);
		// Setting uniform values to the currently bound program
		GLint transform_loc = glGetUniformLocation(currProgram, "transform");
		if (transform_loc >= 0) {
			glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float *)&transform.mat);
		}
		GLint projection_loc = glGetUniformLocation(currProgram, "projection");
		if (projection_loc >= 0) {
			glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection_2D);
		}
		gl_has_errors();
		
		glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_SHORT, nullptr);
		gl_has_errors();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
	// CRITICAL: Clear any pending OpenGL errors from UI rendering
	// This prevents UI errors from crashing the game renderer
	while (glGetError() != GL_NO_ERROR);

    // First, render occlusion mask
    renderOcclusionMask();
    
    // Getting size of window
    int w, h;
    glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// Then render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0);
	glClearDepth(10.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();
	mat3 projection_2D = createProjectionMatrix();
	
	// Render background first (behind everything else)
	for (Entity entity : registry.renderRequests.entities)
	{
		if (!registry.motions.has(entity))
			continue;
		if (registry.renderRequests.get(entity).used_geometry == GEOMETRY_BUFFER_ID::BACKGROUND_QUAD)
		{
			drawTexturedMesh(entity, projection_2D);
		}
	}
	
	// Draw all other textured meshes that have a position and size component
	for (Entity entity : registry.renderRequests.entities)
	{
		if (!registry.motions.has(entity))
			continue;
		if (registry.renderRequests.get(entity).used_geometry != GEOMETRY_BUFFER_ID::BACKGROUND_QUAD)
		{
			// Note, its not very efficient to access elements indirectly via the entity
			// albeit iterating through all Sprites in sequence. A good point to optimize
			drawTexturedMesh(entity, projection_2D);
		}
	}

	// debug: draw player mesh collider, and circle collider
	if (show_player_hitbox_debug) {
		if (g_debug_line_vbo == 0) glGenBuffers(1, &g_debug_line_vbo);
		const GLuint program = effects[(GLuint)EFFECT_ASSET_ID::COLOURED];
		glUseProgram(program);
		GLint posLoc = glGetAttribLocation(program, "in_position");
		GLint colLoc = glGetAttribLocation(program, "in_color");
		GLint transform_loc = glGetUniformLocation(program, "transform");
		GLint projection_loc = glGetUniformLocation(program, "projection");
		mat3 I = { {1,0,0}, {0,1,0}, {0,0,1} };
		if (transform_loc >= 0) glUniformMatrix3fv(transform_loc, 1, GL_FALSE, (float*)&I);
		if (projection_loc >= 0) glUniformMatrix3fv(projection_loc, 1, GL_FALSE, (float *)&projection_2D);

		auto uploadAndDraw = [&](const std::vector<ColoredVertex>& verts){
			glBindBuffer(GL_ARRAY_BUFFER, g_debug_line_vbo);
			glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(verts.size()*sizeof(ColoredVertex)), verts.data(), GL_DYNAMIC_DRAW);
			glEnableVertexAttribArray(posLoc);
			glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)0);
			glEnableVertexAttribArray(colLoc);
			glVertexAttribPointer(colLoc, 3, GL_FLOAT, GL_FALSE, sizeof(ColoredVertex), (void*)sizeof(vec3));
			glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)verts.size());
		};

		auto transformPoints = [&](const std::vector<vec2>& localPts, const Motion& m){
			std::vector<vec2> out(localPts.size());
			float c = cos(m.angle), s = sin(m.angle);
			for (size_t i=0;i<localPts.size();++i)
			{ 
				vec2 p=localPts[i]; 
				p.x*=m.scale.x; p.y*=m.scale.y; 
				vec2 pr={p.x*c - p.y*s, p.x*s + p.y*c}; 
				out[i]=pr + m.position; 
			}
			return out;
		};

		auto drawLineLoop = [&](const std::vector<vec2>& worldPts, vec3 color){
			if (worldPts.size() < 2) return;
			std::vector<ColoredVertex> verts(worldPts.size()+1);
			for (size_t i=0;i<worldPts.size();++i)
			{ 
				verts[i].position={worldPts[i].x,worldPts[i].y,0.f}; 
				verts[i].color=color; 
			}
			verts.back() = verts.front();
			uploadAndDraw(verts);
		};


		auto drawCircle = [&](const Motion& m, float r, vec3 color){
			const int segments = 32;
			const float two_pi = 6.2831853f;
			std::vector<ColoredVertex> verts(segments + 1);
			for (int i = 0; i < segments; ++i)
			{
				float angle = (two_pi * i) / segments;
				float cos_angle = cosf(angle);
				float sin_angle = sinf(angle);
				float x = m.position.x + r * cos_angle;
				float y = m.position.y + r * sin_angle;
				verts[i].position = { x, y, 0.f };
				verts[i].color = color;
			}
			verts[segments] = verts[0];
			uploadAndDraw(verts);
		};

		for (Entity e : registry.players.entities)
		{
			if (!registry.motions.has(e)) continue;
			Motion &m = registry.motions.get(e);
			if (registry.colliders.has(e))
			{
				const CollisionMesh& col = registry.colliders.get(e);
				drawLineLoop(transformPoints(col.local_points, m), {1.f,0.f,0.f});
			}
			
			if (registry.collisionCircles.has(e))
			{
				float r = registry.collisionCircles.get(e).radius;
				drawCircle(m, r, {0.f,0.f,1.f});
			}
		}
	}

	// Truely render to the screen
	drawToScreen();

	// flicker-free display with a double buffer
	// glfwSwapBuffers(window);
	gl_has_errors();
}

mat3 RenderSystem::createProjectionMatrix()
{
	// Fake projection matrix, scales with respect to window coordinates
	float left = 0.f;
	float top = 0.f;

	gl_has_errors();
	float right = (float) window_width_px;
	float bottom = (float) window_height_px;

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	return {{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};
}