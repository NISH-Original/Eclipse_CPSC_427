// internal
#include "render_system.hpp"
#include "low_health_overlay_system.hpp"

#include <array>
#include <fstream>
#include <cmath>

#include "../ext/stb_image/stb_image.h"

// This creates circular header inclusion, that is quite bad.
#include "tiny_ecs_registry.hpp"

// stlib
#include <iostream>
#include <sstream>

// World initialization
bool RenderSystem::init(GLFWwindow* window_arg)
{
	this->window = window_arg;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // vsync

	// Load OpenGL function pointers
	const int is_fine = gl3w_init();
	assert(is_fine == 0);

	// Create a frame buffer
	frame_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// For some high DPI displays (ex. Retina Display on Macbooks)
	// https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value
	int frame_buffer_width_px, frame_buffer_height_px;
	glfwGetFramebufferSize(window, &frame_buffer_width_px, &frame_buffer_height_px);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	// Hint: Ask your TA for how to setup pretty OpenGL error callbacks. 
	// This can not be done in macOS, so do not enable
	// it unless you are on Linux or Windows. You will need to change the window creation
	// code to use OpenGL 4.3 (not suported in macOS) and add additional .h and .cpp
	// glDebugMessageCallback((GLDEBUGPROC)errorCallback, nullptr);

	// We are not really using VAO's but without at least one bound we will crash in
	// some systems.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	gl_has_errors();

	initScreenTexture();
	if (!initShadowTextures()) {
		fprintf(stderr, "Failed to initialize shadow textures\n");
		return false;
	}
	if (!initShadowShaders()) {
		fprintf(stderr, "Failed to initialize shadow shaders\n");
		return false;
	}
    initializeGlTextures();
	initializeGlEffects();
	initializeGlGeometryBuffers();
	
	// Initialize low health overlay system
	low_health_overlay_system = new LowHealthOverlaySystem();
	low_health_overlay_system->init(window, texture_gl_handles, effects, vertex_buffers, index_buffers, nullptr);

	return true;
}

void RenderSystem::initializeGlTextures()
{
    glGenTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());

    for(uint i = 0; i < texture_paths.size(); i++)
    {
		const std::string& path = texture_paths[i];
		ivec2& dimensions = texture_dimensions[i];

		stbi_uc* data;
		data = stbi_load(path.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

		if (data == NULL)
		{
			const std::string message = "Could not load the file " + path + ".";
			fprintf(stderr, "%s", message.c_str());
			assert(false);
		}
		glBindTexture(GL_TEXTURE_2D, texture_gl_handles[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		// Enable texture repeating for grass texture (for tiling)
		if (i == (uint)TEXTURE_ASSET_ID::GRASS) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		
		gl_has_errors();
		stbi_image_free(data);
    }
	gl_has_errors();
}

void RenderSystem::initializeGlEffects()
{
	for(uint i = 0; i < effect_paths.size(); i++)
	{
		const std::string vertex_shader_name = effect_paths[i] + ".vs.glsl";
		const std::string fragment_shader_name = effect_paths[i] + ".fs.glsl";

		bool is_valid = loadEffectFromFile(vertex_shader_name, fragment_shader_name, effects[i]);
		assert(is_valid && (GLuint)effects[i] != 0);
	}
}

// One could merge the following two functions as a template function...
template <class T>
void RenderSystem::bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(uint)gid]);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(uint)gid]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
	gl_has_errors();
}

void RenderSystem::initializeGlMeshes()
{
	for (uint i = 0; i < mesh_paths.size(); i++)
	{
		// Initialize meshes
		GEOMETRY_BUFFER_ID geom_index = mesh_paths[i].first;
		std::string name = mesh_paths[i].second;
		Mesh::loadFromOBJFile(name, 
			meshes[(int)geom_index].vertices,
			meshes[(int)geom_index].vertex_indices,
			meshes[(int)geom_index].original_size);

		bindVBOandIBO(geom_index,
			meshes[(int)geom_index].vertices, 
			meshes[(int)geom_index].vertex_indices);
	}
}

void RenderSystem::initializeGlGeometryBuffers()
{
	// Vertex Buffer creation.
	glGenBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	glGenBuffers(1, &particle_instance_vbo);
	
	// Index Buffer creation.
	glGenBuffers((GLsizei)index_buffers.size(), index_buffers.data());

	// Index and Vertex buffer data initialization.
	initializeGlMeshes();

	// Initialize bullet circle as a small red circle
	const int bullet_segments = 16;
	std::vector<ColoredVertex> bullet_vertices(bullet_segments + 1);
	
	bullet_vertices[0].position = { 0.f, 0.f, 0.f };
	bullet_vertices[0].color = { 1, 0, 0 };
	
	for (int i = 0; i < bullet_segments; i++) {
		float angle = 2.0f * M_PI * i / bullet_segments;
		bullet_vertices[i + 1].position = { 0.3f * cos(angle), 0.3f * sin(angle), 0.f };
		bullet_vertices[i + 1].color = { 1, 0, 0 };
	}

	std::vector<uint16_t> bullet_indices;
	for (int i = 0; i < bullet_segments; i++) {
		bullet_indices.push_back(0);
		bullet_indices.push_back(i + 1);
		bullet_indices.push_back((i + 1) % bullet_segments + 1);
	}

	int bullet_geom_index = (int)GEOMETRY_BUFFER_ID::BULLET_CIRCLE;
	meshes[bullet_geom_index].vertices = bullet_vertices;
	meshes[bullet_geom_index].vertex_indices = bullet_indices;
	meshes[bullet_geom_index].original_size = { 0.6f, 0.6f };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::BULLET_CIRCLE, bullet_vertices, bullet_indices);

	// Initialize enemy as a green triangle with a red tip
	std::vector<ColoredVertex> enemy_vertices(3);
	enemy_vertices[0].position = { -0.433f, -0.5f, 0.f };
	enemy_vertices[0].color = { 0, 1, 0 }; // Green 
	enemy_vertices[1].position = { -0.433f, +0.5f, 0.f };
	enemy_vertices[1].color = { 0, 1, 0 }; // Green
	enemy_vertices[2].position = { +0.433f, +0.0f, 0.f };
	enemy_vertices[2].color = { 1, 0, 0 }; // Red
	
	const std::vector<uint16_t> enemy_indices = { 0, 1, 2 };

	int enemy_geom_index = (int)GEOMETRY_BUFFER_ID::ENEMY_TRIANGLE;
	meshes[enemy_geom_index].vertices = enemy_vertices;
	meshes[enemy_geom_index].vertex_indices = enemy_indices;
	meshes[enemy_geom_index].original_size = { 1.0f, 1.0f }; // Set original size
	bindVBOandIBO(GEOMETRY_BUFFER_ID::ENEMY_TRIANGLE, enemy_vertices, enemy_indices);

	// Initialize arrow triangle - white triangle pointing right (will be rotated to point at bonfire)
	std::vector<ColoredVertex> arrow_vertices(3);
	arrow_vertices[0].position = { 0.6f, 0.0f, 0.f };  // Tip pointing right
	arrow_vertices[0].color = { 1, 1, 1 }; // White
	arrow_vertices[1].position = { -0.3f, -0.25f, 0.f }; // Bottom left (thinner base)
	arrow_vertices[1].color = { 1, 1, 1 }; // White
	arrow_vertices[2].position = { -0.3f, +0.25f, 0.f }; // Top left (thinner base)
	arrow_vertices[2].color = { 1, 1, 1 }; // White
	
	const std::vector<uint16_t> arrow_indices = { 0, 1, 2 };

	int arrow_geom_index = (int)GEOMETRY_BUFFER_ID::ARROW_TRIANGLE;
	meshes[arrow_geom_index].vertices = arrow_vertices;
	meshes[arrow_geom_index].vertex_indices = arrow_indices;
	meshes[arrow_geom_index].original_size = { 1.0f, 1.0f }; // Set original size
	bindVBOandIBO(GEOMETRY_BUFFER_ID::ARROW_TRIANGLE, arrow_vertices, arrow_indices);


	//////////////////////////
	// Initialize sprite
	// The position corresponds to the center of the texture.
	std::vector<TexturedVertex> textured_vertices(4);
	textured_vertices[0].position = { -1.f/2, +1.f/2, 0.f };
	textured_vertices[1].position = { +1.f/2, +1.f/2, 0.f };
	textured_vertices[2].position = { +1.f/2, -1.f/2, 0.f };
	textured_vertices[3].position = { -1.f/2, -1.f/2, 0.f };
	textured_vertices[0].texcoord = { 0.f, 1.f };
	textured_vertices[1].texcoord = { 1.f, 1.f };
	textured_vertices[2].texcoord = { 1.f, 0.f };
	textured_vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> textured_indices = { 0, 3, 1, 1, 3, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SPRITE, textured_vertices, textured_indices);


	///////////////////////////////////////////////////////
	// Initialize screen triangle (yes, triangle, not quad; its more efficient).
	std::vector<vec3> screen_vertices(3);
	screen_vertices[0] = { -1, -6, 0.f };
	screen_vertices[1] = { 6, -1, 0.f };
	screen_vertices[2] = { -1, 6, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> screen_indices = { 0, 1, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE, screen_vertices, screen_indices);

	///////////////////////////////////////////////////////
	// Initialize background quad for grass tiling
	std::vector<TexturedVertex> background_vertices(4);
	background_vertices[0].position = { -1.f, -1.f, 0.f };
	background_vertices[1].position = { +1.f, -1.f, 0.f };
	background_vertices[2].position = { +1.f, +1.f, 0.f };
	background_vertices[3].position = { -1.f, +1.f, 0.f };
	background_vertices[0].texcoord = { 0.f, 0.f };
	background_vertices[1].texcoord = { 2000.f, 0.f };
	background_vertices[2].texcoord = { 2000.f, 2000.f };
	background_vertices[3].texcoord = { 0.f, 2000.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> background_indices = { 0, 1, 2, 0, 2, 3 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::BACKGROUND_QUAD, background_vertices, background_indices);

	// Fullscreen quad for SDF and point light passes
	std::vector<TexturedVertex> fullscreen_vertices(4);
	fullscreen_vertices[0].position = { -1.f, -1.f, 0.f };
	fullscreen_vertices[1].position = { +1.f, -1.f, 0.f };
	fullscreen_vertices[2].position = { +1.f, +1.f, 0.f };
	fullscreen_vertices[3].position = { -1.f, +1.f, 0.f };
	fullscreen_vertices[0].texcoord = { 0.f, 0.f };
	fullscreen_vertices[1].texcoord = { 1.f, 0.f };
	fullscreen_vertices[2].texcoord = { 1.f, 1.f };
	fullscreen_vertices[3].texcoord = { 0.f, 1.f };

	const std::vector<uint16_t> fullscreen_indices = { 0, 1, 2, 0, 2, 3 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::FULLSCREEN_QUAD, fullscreen_vertices, fullscreen_indices);

	///////////////////////////////////////////////////////
	// Initialize health bar geometry
	std::vector<ColoredVertex> healthbar_vertices(4);
	healthbar_vertices[0].position = { 0.f, 0.f, 0.f };
	healthbar_vertices[1].position = { 1.f, 0.f, 0.f };
	healthbar_vertices[2].position = { 1.f, 1.f, 0.f };
	healthbar_vertices[3].position = { 0.f, 1.f, 0.f };
	healthbar_vertices[0].color = { 1.0f, 1.0f, 1.0f }; // White color (will be overridden)
	healthbar_vertices[1].color = { 1.0f, 1.0f, 1.0f };
	healthbar_vertices[2].color = { 1.0f, 1.0f, 1.0f };
	healthbar_vertices[3].color = { 1.0f, 1.0f, 1.0f };

	const std::vector<uint16_t> healthbar_indices = { 0, 1, 2, 0, 2, 3 };

	int healthbar_geom_index = (int)GEOMETRY_BUFFER_ID::HEALTH_BAR;
	meshes[healthbar_geom_index].vertices = healthbar_vertices;
	meshes[healthbar_geom_index].vertex_indices = healthbar_indices;
	meshes[healthbar_geom_index].original_size = { 1.0f, 1.0f };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::HEALTH_BAR, healthbar_vertices, healthbar_indices);
}

RenderSystem::~RenderSystem()
{
	// Clean up low health overlay system
	if (low_health_overlay_system) {
		delete low_health_overlay_system;
		low_health_overlay_system = nullptr;
	}
	// Don't need to free gl resources since they last for as long as the program,
	// but it's polite to clean after yourself.
	glDeleteBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	glDeleteBuffers((GLsizei)index_buffers.size(), index_buffers.data());
	glDeleteTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());
	glDeleteTextures(1, &off_screen_render_buffer_color);
	glDeleteRenderbuffers(1, &off_screen_render_buffer_depth);
	glDeleteTextures(1, &scene_texture);
	glDeleteFramebuffers(1, &scene_fb);
	glDeleteTextures(1, &sdf_voronoi_texture1);
	glDeleteFramebuffers(1, &sdf_voronoi_fb1);
	glDeleteTextures(1, &sdf_voronoi_texture2);
	glDeleteFramebuffers(1, &sdf_voronoi_fb2);
	glDeleteTextures(1, &sdf_texture);
	glDeleteFramebuffers(1, &sdf_fb);
	glDeleteTextures(1, &lighting_texture);
	glDeleteFramebuffers(1, &lighting_fb);
	glDeleteProgram(sdf_seed_program);
	glDeleteProgram(sdf_jump_flood_program);
	glDeleteProgram(sdf_distance_program);
	gl_has_errors();

	for(uint i = 0; i < effect_count; i++) {
		glDeleteProgram(effects[i]);
	}
	// delete allocated resources
	glDeleteFramebuffers(1, &frame_buffer);
	gl_has_errors();

	// remove all entities created by the render system
	while (registry.renderRequests.entities.size() > 0)
	    registry.remove_all_components_of(registry.renderRequests.entities.back());
}

// Initialize the screen texture from a standard sprite
bool RenderSystem::initScreenTexture()
{
	registry.screenStates.emplace(screen_state_entity);

	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &framebuffer_width, &framebuffer_height);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	glGenTextures(1, &off_screen_render_buffer_color);
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();

	glGenRenderbuffers(1, &off_screen_render_buffer_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, off_screen_render_buffer_depth);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, off_screen_render_buffer_color, 0);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, framebuffer_width, framebuffer_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, off_screen_render_buffer_depth);
	gl_has_errors();

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	return true;
}

bool gl_compile_shader(GLuint shader)
{
	glCompileShader(shader);
	gl_has_errors();
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		GLint log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> log(log_len);
		glGetShaderInfoLog(shader, log_len, &log_len, log.data());
		glDeleteShader(shader);

		gl_has_errors();

		fprintf(stderr, "GLSL: %s", log.data());
		return false;
	}

	return true;
}

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program)
{
	// Opening files
	std::ifstream vs_is(vs_path);
	std::ifstream fs_is(fs_path);
	if (!vs_is.good() || !fs_is.good())
	{
		fprintf(stderr, "Failed to load shader files %s, %s", vs_path.c_str(), fs_path.c_str());
		assert(false);
		return false;
	}

	// Reading sources
	std::stringstream vs_ss, fs_ss;
	vs_ss << vs_is.rdbuf();
	fs_ss << fs_is.rdbuf();
	std::string vs_str = vs_ss.str();
	std::string fs_str = fs_ss.str();
	const char* vs_src = vs_str.c_str();
	const char* fs_src = fs_str.c_str();
	GLsizei vs_len = (GLsizei)vs_str.size();
	GLsizei fs_len = (GLsizei)fs_str.size();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vs_src, &vs_len);
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fs_src, &fs_len);
	gl_has_errors();

	// Compiling
	if (!gl_compile_shader(vertex))
	{
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}
	if (!gl_compile_shader(fragment))
	{
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}

	// Linking
	out_program = glCreateProgram();
	glAttachShader(out_program, vertex);
	glAttachShader(out_program, fragment);
	glLinkProgram(out_program);
	gl_has_errors();

	{
		GLint is_linked = GL_FALSE;
		glGetProgramiv(out_program, GL_LINK_STATUS, &is_linked);
		if (is_linked == GL_FALSE)
		{
			GLint log_len;
			glGetProgramiv(out_program, GL_INFO_LOG_LENGTH, &log_len);
			std::vector<char> log(log_len);
			glGetProgramInfoLog(out_program, log_len, &log_len, log.data());
			gl_has_errors();

			fprintf(stderr, "Link error: %s", log.data());
			assert(false);
			return false;
		}
	}

	// No need to carry this around. Keeping these objects is only useful if we recycle
	// the same shaders over and over, which we don't, so no need and this is simpler.
	glDetachShader(out_program, vertex);
	glDetachShader(out_program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	gl_has_errors();

	return true;
}

bool RenderSystem::initShadowTextures()
{
	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &framebuffer_width, &framebuffer_height);

	// SDF Shadow System Textures
	glGenTextures(1, &scene_texture);
	glBindTexture(GL_TEXTURE_2D, scene_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1, &scene_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, scene_fb);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, scene_texture, 0);
	gl_has_errors();

	// Jump Texture 1
	glGenTextures(1, &sdf_voronoi_texture1);
	glBindTexture(GL_TEXTURE_2D, sdf_voronoi_texture1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1, &sdf_voronoi_fb1);
	glBindFramebuffer(GL_FRAMEBUFFER, sdf_voronoi_fb1);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sdf_voronoi_texture1, 0);
	gl_has_errors();

	// Jump Texture 2
	glGenTextures(1, &sdf_voronoi_texture2);
	glBindTexture(GL_TEXTURE_2D, sdf_voronoi_texture2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1, &sdf_voronoi_fb2);
	glBindFramebuffer(GL_FRAMEBUFFER, sdf_voronoi_fb2);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sdf_voronoi_texture2, 0);
	gl_has_errors();

	// Distance Field Texture
	glGenTextures(1, &sdf_texture);
	glBindTexture(GL_TEXTURE_2D, sdf_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1, &sdf_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, sdf_fb);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, sdf_texture, 0);
	gl_has_errors();

	// Temporary Texture
	glGenTextures(1, &lighting_texture);
	glBindTexture(GL_TEXTURE_2D, lighting_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenFramebuffers(1, &lighting_fb);
	glBindFramebuffer(GL_FRAMEBUFFER, lighting_fb);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lighting_texture, 0);
	gl_has_errors();
 
	return true;
}

bool RenderSystem::initShadowShaders()
{
	// Screen UV Shader
	fprintf(stderr, "Loading SDF shadow shaders...\n");
	if (!loadEffectFromFile(shader_path("screen.vs.glsl"), shader_path("sdf_seed.fs.glsl"), sdf_seed_program)) {
		fprintf(stderr, "Failed to load screen_uv shader\n");
		return false;
	}
	// Jump Flood Shader
	fprintf(stderr, "Loaded screen_uv shader\n");
	if (!loadEffectFromFile(shader_path("screen.vs.glsl"), shader_path("sdf_jump_flood.fs.glsl"), sdf_jump_flood_program)) {
		fprintf(stderr, "Failed to load jump_flood shader\n");
		return false;
	}
	// Distance Field Shader
	fprintf(stderr, "Loaded jump_flood shader\n");
	if (!loadEffectFromFile(shader_path("screen.vs.glsl"), shader_path("sdf_distance.fs.glsl"), sdf_distance_program)) {
		fprintf(stderr, "Failed to load distance_field shader\n");
		return false;
	}
	// Point Light Shader
	fprintf(stderr, "Loaded distance_field shader\n");
	if (!loadEffectFromFile(shader_path("screen.vs.glsl"), shader_path("point_light.fs.glsl"), point_light_program)) {
		fprintf(stderr, "Failed to load point light shader\n");
		return false;
	}

	fprintf(stderr, "Loaded point light shader\n");
	fprintf(stderr, "All SDF shadow shaders loaded successfully!\n");

	return true;
}
