// internal
#include "render_system.hpp"
#include <SDL.h>
#include <iostream>

#include "tiny_ecs_registry.hpp"

static GLuint g_debug_line_vbo = 0;

inline unsigned char state_to_iso_bitmap(CHUNK_CELL_STATE state) {
	switch (state) {
		case CHUNK_CELL_STATE::ISO_01: return 1;
		case CHUNK_CELL_STATE::ISO_02: return 2;
		case CHUNK_CELL_STATE::ISO_03: return 3;
		case CHUNK_CELL_STATE::ISO_04: return 4;
		case CHUNK_CELL_STATE::ISO_05: return 5;
		case CHUNK_CELL_STATE::ISO_06: return 6;
		case CHUNK_CELL_STATE::ISO_07: return 7;
		case CHUNK_CELL_STATE::ISO_08: return 8;
		case CHUNK_CELL_STATE::ISO_09: return 9;
		case CHUNK_CELL_STATE::ISO_10: return 10;
		case CHUNK_CELL_STATE::ISO_11: return 11;
		case CHUNK_CELL_STATE::ISO_12: return 12;
		case CHUNK_CELL_STATE::ISO_13: return 13;
		case CHUNK_CELL_STATE::ISO_14: return 14;
		case CHUNK_CELL_STATE::ISO_15: return 15;
		default: return 0;
	}
}

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

		GLint total_row_uloc = glGetUniformLocation(program, "total_row");
		assert(total_row_uloc >= 0);
		glUniform1i(total_row_uloc, sprite.total_row);

		GLint curr_row_uloc = glGetUniformLocation(program, "curr_row");
		assert(curr_row_uloc >= 0);
		glUniform1i(curr_row_uloc, sprite.curr_row);

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

		// Pass viewport size for screen UV calculation
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		GLint viewport_loc = glGetUniformLocation(program, "viewport_size");
		if (viewport_loc >= 0) glUniform2f(viewport_loc, (float)w, (float)h);
		gl_has_errors();

		// Pass ambient light level
		GLint ambient_loc = glGetUniformLocation(program, "ambient_light");
		if (ambient_loc >= 0) glUniform1f(ambient_loc, 0.3f);  // Base ambient lighting
		gl_has_errors();
	}
	else if (render_request.used_effect == EFFECT_ASSET_ID::COLOURED)
	{
		vec3 white = {1.0f, 1.0f, 1.0f};
		GLint fcolor_uloc = glGetUniformLocation(program, "fcolor");
		if (fcolor_uloc >= 0) glUniform3fv(fcolor_uloc, 1, (float*)&white);

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

void RenderSystem::drawIsocell(vec2 position, const mat3 &projection)
{
	Transform transform;
	transform.translate(position);
	//transform.rotate(0);
	transform.scale(vec2(CHUNK_CELL_SIZE, CHUNK_CELL_SIZE));

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

// TODO: speed up rendering (currently VERY laggy)
void RenderSystem::drawChunks(const mat3 &projection)
{
	vec4 cam_view = getCameraView();
	float cells_per_row = (float) CHUNK_CELLS_PER_ROW;
	float cell_size = (float) CHUNK_CELL_SIZE;

	// Setting shaders
	const GLuint used_effect_enum = (GLuint) EFFECT_ASSET_ID::TILED;
	const GLuint program = (GLuint)effects[used_effect_enum];
	glUseProgram(program);
	gl_has_errors();

	// Setting vertex and index buffers
	const GLuint vbo = vertex_buffers[(GLuint) GEOMETRY_BUFFER_ID::SPRITE];
	const GLuint ibo = index_buffers[(GLuint) GEOMETRY_BUFFER_ID::SPRITE];
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	gl_has_errors();

	GLint tot_states_uloc = glGetUniformLocation(program, "total_states");
	//GLint tex_states_uloc = glGetUniformLocation(program, "tex_states");
	GLint s_bit_uloc = glGetUniformLocation(program, "s_bit");

	assert(tot_states_uloc >= 0);
	//assert(tex_states_uloc >= 0);
	assert(s_bit_uloc >= 0);
	glUniform1i(tot_states_uloc, 16);
	gl_has_errors();

	GLint in_position_loc = glGetAttribLocation(program, "in_position");
	GLint in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	gl_has_errors();
	assert(in_texcoord_loc >= 0);

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE,
							sizeof(TexturedVertex), (void *)0);
	glEnableVertexAttribArray(in_texcoord_loc);
	glVertexAttribPointer(
		in_texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex),
		(void *)sizeof(
			vec3)); // note the stride to skip the preceeding vertex position
	gl_has_errors();

	glActiveTexture(GL_TEXTURE0);
	GLuint texture_id = texture_gl_handles[(GLuint) TEXTURE_ASSET_ID::ISOROCK];
	glBindTexture(GL_TEXTURE_2D, texture_id);
	gl_has_errors();

	// Pass viewport size for screen UV calculation
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	GLint viewport_loc = glGetUniformLocation(program, "viewport_size");
	if (viewport_loc >= 0) glUniform2f(viewport_loc, (float)w, (float)h);
	gl_has_errors();

	// Pass ambient light level
	GLint ambient_loc = glGetUniformLocation(program, "ambient_light");
	if (ambient_loc >= 0) glUniform1f(ambient_loc, 0.3f);  // Base ambient lighting
	gl_has_errors();

	// Getting uniform locations for glUniform* calls
	GLint color_uloc = glGetUniformLocation(program, "fcolor");
	const vec3 color = vec3(1);
	glUniform3fv(color_uloc, 1, (float *)&color);
	gl_has_errors();

	for (size_t n = 0; n < registry.chunks.size(); n++) {
		short chunk_pos_x = registry.chunks.position_xs[n];
		short chunk_pos_y = registry.chunks.position_ys[n];
		Chunk& chunk = registry.chunks.components[n];
		vec2 base_pos = vec2(chunk_pos_x*cells_per_row*cell_size, chunk_pos_y*cells_per_row*cell_size);
		
		for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i += 1) {
			/*if (base_pos.x + (i+1)*CHUNK_CELL_SIZE < cam_view.x ||
				base_pos.x + i*CHUNK_CELL_SIZE > cam_view.y)
			{
				continue;
			}*/
			
			for (size_t j = 0; j < CHUNK_CELLS_PER_ROW; j += 1) {
				// TODO: fix checks
				/*if (base_pos.y + (j+1)*CHUNK_CELL_SIZE < cam_view.z ||
					base_pos.y + j*CHUNK_CELL_SIZE > cam_view.w)
				{
					continue;
				}*/

				//mat4 tex_states = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
				/*mat4 tex_states = {
					{
						state_to_iso_bitmap(chunk.cell_states[i][j]),
						state_to_iso_bitmap(chunk.cell_states[i][j+1]),
						state_to_iso_bitmap(chunk.cell_states[i][j+2]),
						state_to_iso_bitmap(chunk.cell_states[i][j+3])
					},
					{
						state_to_iso_bitmap(chunk.cell_states[i+1][j]),
						state_to_iso_bitmap(chunk.cell_states[i+1][j+1]),
						state_to_iso_bitmap(chunk.cell_states[i+1][j+2]),
						state_to_iso_bitmap(chunk.cell_states[i+1][j+3])
					},
					{
						state_to_iso_bitmap(chunk.cell_states[i+2][j]),
						state_to_iso_bitmap(chunk.cell_states[i+2][j+2]),
						state_to_iso_bitmap(chunk.cell_states[i+2][j+2]),
						state_to_iso_bitmap(chunk.cell_states[i+2][j+3])
					},
					{
						state_to_iso_bitmap(chunk.cell_states[i+3][j]),
						state_to_iso_bitmap(chunk.cell_states[i+3][j+1]),
						state_to_iso_bitmap(chunk.cell_states[i+3][j+2]),
						state_to_iso_bitmap(chunk.cell_states[i+3][j+3])
					}
				};*/

				unsigned char s_bit = state_to_iso_bitmap(chunk.cell_states[i][j]);
				if (s_bit != 0) {
					//glUniformMatrix4fv(tex_states_uloc, 1, GL_FALSE, (float*)&tex_states);
					glUniform1i(s_bit_uloc, s_bit);
					vec2 pos = base_pos + vec2(i*cell_size + cell_size/2 , j*cell_size + cell_size/2);
					drawIsocell(pos, projection);
				}
			}
		}
	}
}

// draw the intermediate texture to the screen, with some distortion to simulate
// water
void RenderSystem::drawToScreen()
{
	// Screen UV Shader
	glUseProgram(effects[(GLuint)EFFECT_ASSET_ID::SCREEN]);
	gl_has_errors();

	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, w, h);
	glDepthRange(0, 10);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0);
	glClearDepth(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gl_has_errors();

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(GLuint)GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE]);
	gl_has_errors();

	// Get the shader program used for rendering the final screen quad
	const GLuint screen_program = effects[(GLuint)EFFECT_ASSET_ID::SCREEN];

	GLint in_position_loc = glGetAttribLocation(screen_program, "in_position");
	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void *)0);
	gl_has_errors();

	glActiveTexture(GL_TEXTURE0);

	// Bind the frame buffer texture (final lit scene) to the screen texture slot
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);

	GLint screen_texture_uloc = glGetUniformLocation(screen_program, "screen_texture");
	
	// Set the screen texture slot to the texture we bound
	glUniform1i(screen_texture_uloc, 0);
	gl_has_errors();

	// Draw the screen quad
	glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr);
	gl_has_errors();
}

// Render our game world
// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
void RenderSystem::draw()
{
	// CRITICAL: Clear any pending OpenGL errors from UI rendering
	// This prevents UI errors from crashing the game renderer
	while (glGetError() != GL_NO_ERROR);

	// Getting size of window
	int w, h;
	glfwGetFramebufferSize(window, &w, &h); // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// Render to the custom framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();
	// Clearing backbuffer
	glViewport(0, 0, w, h);
	glDepthRange(0.00001, 10);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0);
	glClearDepth(10.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST); // native OpenGL does not work with a depth buffer
							  // and alpha blending, one would have to sort
							  // sprites back to front
	gl_has_errors();

	vec4 cam_view = getCameraView();
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

	// debug: these 3 are moved here so that the debug containers are drawn on top of everything
	renderSceneToColorTexture();
	renderLightingWithShadows();
	drawToScreen();
	
	if (show_player_hitbox_debug) {
		int w, h;
		glfwGetFramebufferSize(window, &w, &h);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, w, h);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);
		
		if (g_debug_line_vbo == 0) glGenBuffers(1, &g_debug_line_vbo);
		const GLuint program = effects[(GLuint)EFFECT_ASSET_ID::COLOURED];
		glUseProgram(program);
		GLint posLoc = glGetAttribLocation(program, "in_position");
		GLint colLoc = glGetAttribLocation(program, "in_color");
		GLint transform_loc = glGetUniformLocation(program, "transform");
		GLint projection_loc = glGetUniformLocation(program, "projection");
		mat3 I = { {1,0,0}, {0,1,0}, {0,0,1} };
		mat3 projection_2D = createProjectionMatrix();
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

	gl_has_errors();
}

// vector of 4 components: LEFT, RIGHT, TOP, BOTTOM
vec4 RenderSystem::getCameraView()
{
	// Calculate half the width and height of the window
	float half_width = (float)window_width_px / 2.f;
	float half_height = (float)window_height_px / 2.f;

	// Calculate the left, right, top, and bottom edges of the camera's view
	// This lets us center the view around the camera position
	float left = camera_position.x - half_width;
	float right = camera_position.x + half_width;
	float top = camera_position.y - half_height;
	float bottom = camera_position.y + half_height;

	return vec4(left, right, top, bottom);
}

mat3 RenderSystem::createProjectionMatrix()
{
	vec4 cam_view = getCameraView();
	float left = cam_view.x;
	float right = cam_view.y;
	float top = cam_view.z;
	float bottom = cam_view.w;

	gl_has_errors();

	float sx = 2.f / (right - left);
	float sy = 2.f / (top - bottom);
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	return {{sx, 0.f, 0.f}, {0.f, sy, 0.f}, {tx, ty, 1.f}};
}

void RenderSystem::renderSceneToColorTexture()
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);

	glBindFramebuffer(GL_FRAMEBUFFER, scene_fb);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	mat3 projection_2D = createProjectionMatrix();
	vec4 cam_view = getCameraView();

	// Render chunk-based data (i.e. isoline obstacles)
	// NOTE: currently not supported by world gen
	//drawChunks(projection_2D);

	// Loop through all entities and render them to the color texture
	for (Entity entity : registry.renderRequests.entities)
	{
		if (!registry.motions.has(entity))
			continue;
		if (registry.renderRequests.get(entity).used_geometry == GEOMETRY_BUFFER_ID::BACKGROUND_QUAD)
			continue;

		// Do not draw entities that are off-screen
		Motion& m = registry.motions.get(entity);
		if (m.position.x + abs(m.scale.x) < cam_view.x ||
			m.position.x - abs(m.scale.x) > cam_view.y ||
			m.position.y + abs(m.scale.y) < cam_view.z ||
			m.position.y - abs(m.scale.y) > cam_view.w)
		{
			continue;
		}

		drawTexturedMesh(entity, projection_2D);
	}

	gl_has_errors();
}

// Generates soft shadows using a signed distance field
// 1. Generate SDF seeds from occluders
// 2. Run Jump Flood Algorithm to create Voronoi diagram
// 3. Convert Voronoi diagram to distance field
// 4. Render point lights with soft shadows using the SDF
// This was originally a radiance cascade pipeline
// based on https://github.com/Hybrid46/RadianceCascade2DGlobalIllumination
void RenderSystem::renderLightingWithShadows()
{
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	GLuint quad_vbo = vertex_buffers[(GLuint)GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD];
	GLuint quad_ibo = index_buffers[(GLuint)GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD];

	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ibo);

	// Query attribute location (screen.vs.glsl only takes position, generates texcoords)
	GLint in_position_loc = glGetAttribLocation(sdf_seed_program, "in_position");

	glEnableVertexAttribArray(in_position_loc);
	glVertexAttribPointer(in_position_loc, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);

	// STEP 1: Generate SDF seeds from occluders
	glBindFramebuffer(GL_FRAMEBUFFER, sdf_voronoi_fb1);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(sdf_seed_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene_texture);
	GLint uv_scene_loc = glGetUniformLocation(sdf_seed_program, "scene_texture");
	if (uv_scene_loc >= 0) glUniform1i(uv_scene_loc, 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

	// Build Voronoi from seeds
	int max_steps = (int)ceil(log2(fmax(w, h)));
	GLuint read_tex = sdf_voronoi_texture1;
	GLuint write_fb = sdf_voronoi_fb2;
	GLuint write_tex = sdf_voronoi_texture2;

	glUseProgram(sdf_jump_flood_program);
	GLint jf_prev_loc = glGetUniformLocation(sdf_jump_flood_program, "previous_texture");
	GLint jf_step_loc = glGetUniformLocation(sdf_jump_flood_program, "step_size");
	GLint jf_aspect_loc = glGetUniformLocation(sdf_jump_flood_program, "aspect");


	for (int i = max_steps - 1; i >= 0; i--)
	{
		float step_size = pow(2.0f, (float)i);

		glBindFramebuffer(GL_FRAMEBUFFER, write_fb);
		glViewport(0, 0, w, h);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, read_tex);
		if (jf_prev_loc >= 0) glUniform1i(jf_prev_loc, 0);
		if (jf_step_loc >= 0) glUniform1f(jf_step_loc, step_size);
		if (jf_aspect_loc >= 0) glUniform2f(jf_aspect_loc, 1.0f / (float)w, 1.0f / (float)h);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

		GLuint temp = read_tex;
		read_tex = write_tex;
		write_tex = temp;

		GLuint temp_fb = write_fb;
		write_fb = (write_fb == sdf_voronoi_fb2) ? sdf_voronoi_fb1 : sdf_voronoi_fb2;
	}

	// Convert Voronoi diagram to a distance field
	// Effectively a map of the distance to the nearest occluder
	glBindFramebuffer(GL_FRAMEBUFFER, sdf_fb);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(sdf_distance_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, read_tex);
	GLint df_voronoi_loc = glGetUniformLocation(sdf_distance_program, "voronoi_texture");
	if (df_voronoi_loc >= 0) glUniform1i(df_voronoi_loc, 0);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

	// Render point lights with soft shadows using our sdf map
	glBindFramebuffer(GL_FRAMEBUFFER, lighting_fb);
	glViewport(0, 0, w, h);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	glUseProgram(point_light_program);

	// Get the inputs for the point light shader
	GLint pl_scene_loc = glGetUniformLocation(point_light_program, "scene_texture");
	GLint pl_sdf_loc = glGetUniformLocation(point_light_program, "sdf_texture");
	GLint pl_light_pos_loc = glGetUniformLocation(point_light_program, "light_position");
	GLint pl_light_color_loc = glGetUniformLocation(point_light_program, "light_color");
	GLint pl_light_radius_loc = glGetUniformLocation(point_light_program, "light_radius");
	GLint pl_screen_size_loc = glGetUniformLocation(point_light_program, "screen_size");
	GLint pl_flicker_loc = glGetUniformLocation(point_light_program, "flicker_intensity");
	GLint pl_time_loc = glGetUniformLocation(point_light_program, "time");
	GLint pl_direction_loc = glGetUniformLocation(point_light_program, "light_direction");
	GLint pl_cone_angle_loc = glGetUniformLocation(point_light_program, "cone_angle");
	GLint pl_height_loc = glGetUniformLocation(point_light_program, "light_height");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, scene_texture);
	if (pl_scene_loc >= 0) glUniform1i(pl_scene_loc, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, sdf_texture);
	if (pl_sdf_loc >= 0) glUniform1i(pl_sdf_loc, 1);

	if (pl_screen_size_loc >= 0) glUniform2f(pl_screen_size_loc, (float)w, (float)h);

	float time = (float)glfwGetTime();
	if (pl_time_loc >= 0) glUniform1f(pl_time_loc, time);

	// Loop through all lights
	for (Entity entity : registry.lights.entities)
	{
		if (!registry.motions.has(entity)) continue;

		Motion& motion = registry.motions.get(entity);
		Light& light = registry.lights.get(entity);

		// Get the lights component settings
		float radius = light.range;
		vec3 color = light.light_color;
		float flicker = 1.0f;
		float coneAngle = light.cone_angle;
		vec2 direction = vec2(1.0f, 0.0f);

		if (light.use_target_angle) {
			direction = vec2(cos(motion.angle), sin(motion.angle));
		}

		if (registry.bullets.has(entity)) {
			radius = 70.0f;
			color *= 0.5f;
			coneAngle = 3.14159f;
		}

		float lightHeight = 0.4f;

		vec2 screen_light_pos;
		screen_light_pos.x = motion.position.x - camera_position.x + (w / 2.0f);
		screen_light_pos.y = motion.position.y - camera_position.y + (h / 2.0f);

		// Set the inputs for the point light shader
		if (pl_light_pos_loc >= 0) glUniform2f(pl_light_pos_loc, screen_light_pos.x, screen_light_pos.y);
		if (pl_light_color_loc >= 0) glUniform3f(pl_light_color_loc, color.r, color.g, color.b);
		if (pl_light_radius_loc >= 0) glUniform1f(pl_light_radius_loc, radius);
		if (pl_flicker_loc >= 0) glUniform1f(pl_flicker_loc, flicker);
		if (pl_direction_loc >= 0) glUniform2f(pl_direction_loc, direction.x, direction.y);
		if (pl_cone_angle_loc >= 0) glUniform1f(pl_cone_angle_loc, coneAngle);
		if (pl_height_loc >= 0) glUniform1f(pl_height_loc, lightHeight);

		// Draw the point light
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
	}

	glDisable(GL_BLEND);

	glDisableVertexAttribArray(in_position_loc);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, lighting_fb);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frame_buffer);
	glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glEnable(GL_BLEND);
}
