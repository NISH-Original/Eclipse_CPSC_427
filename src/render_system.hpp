#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

// Forward declaration
class LowHealthOverlaySystem;

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem {
	/**
	 * The following arrays store the assets the game will use. They are loaded
	 * at initialization and are assumed to not be modified by the render loop.
	 *
	 * Whenever possible, add to these lists instead of creating dynamic state
	 * it is easier to debug and faster to execute for the computer.
	 */
	std::array<GLuint, texture_count> texture_gl_handles;
	std::array<ivec2, texture_count> texture_dimensions;

	// Make sure these paths remain in sync with the associated enumerators.
	// Associated id with .obj path
	const std::vector < std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths =
	{
		  // specify meshes of other assets here
	};

	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, texture_count> texture_paths = {
		textures_path("xylarite.png"),
		textures_path("Enemies/xylarite_crab.png"),
		textures_path("Enemies/slime_1.png"),
		textures_path("Enemies/slime_2.png"),
		textures_path("Enemies/slime_3.png"),
		textures_path("Enemies/Plant_Idle_1.png"),
		textures_path("Enemies/Plant_Attack_1.png"),
		textures_path("Enemies/Plant_Hurt_1.png"),
		textures_path("Enemies/Plant_Death_1.png"),
		textures_path("Enemies/Plant_Idle_2.png"),
		textures_path("Enemies/Plant_Attack_2.png"),
		textures_path("Enemies/Plant_Hurt_2.png"),
		textures_path("Enemies/Plant_Death_2.png"),
		textures_path("Enemies/Plant_Idle_3.png"),
		textures_path("Enemies/Plant_Attack_3.png"),
		textures_path("Enemies/Plant_Hurt_3.png"),
		textures_path("Enemies/Plant_Death_3.png"),
		textures_path("tree.png"),
		textures_path("Player/Handgun/idle.png"),
		textures_path("Player/Handgun/move.png"),
		textures_path("Player/Handgun/shoot.png"),
		textures_path("Player/Handgun/reload.png"),
		textures_path("Player/Shotgun/idle.png"),
		textures_path("Player/Shotgun/move.png"),
		textures_path("Player/Shotgun/shoot.png"),
		textures_path("Player/Shotgun/reload.png"),
		textures_path("Player/Rifle/idle.png"),
		textures_path("Player/Rifle/move.png"),
		textures_path("Player/Rifle/shoot.png"),
		textures_path("Player/Rifle/reload.png"),
		textures_path("Player/Handgun/hurt.png"),
		textures_path("Player/Shotgun/hurt.png"),
		textures_path("Player/Rifle/hurt.png"),
		textures_path("Feet/walk.png"),
		textures_path("Feet/left.png"),
		textures_path("Feet/right.png"),
		textures_path("Dash/dash.png"),
		textures_path("bonfire.png"),
		textures_path("bonfire_off.png"),
		textures_path("arrow_2.png"),
		textures_path("rock_sheet.png"),
		textures_path("grass.png"),
		textures_path("low_health_blood.png")
	};

	std::array<GLuint, effect_count> effects;
	// Make sure these paths remain in sync with the associated enumerators.
	const std::array<std::string, effect_count> effect_paths = {
		shader_path("coloured"),
		shader_path("textured"),
		shader_path("screen"),
		shader_path("tiled"),
		shader_path("healthbar"),
		shader_path("particle"),
	};

	std::array<GLuint, geometry_count> vertex_buffers;
	std::array<GLuint, geometry_count> index_buffers;
	std::array<Mesh, geometry_count> meshes;

public:
	// Initialize the window
	bool init(GLFWwindow* window);
	
	// Set health system for low health overlay
	void set_health_system(class HealthSystem* health_system);

	// global world lighting
	float global_ambient_brightness = 0.01f;

	void setGlobalAmbientBrightness(float brightness) {
		global_ambient_brightness = brightness;
	}

	template <class T>
	void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

	void initializeGlTextures();

	void initializeGlEffects();

	void initializeGlMeshes();
	Mesh& getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

	void initializeGlGeometryBuffers();
	// Initialize the screen texture used as intermediate render target
	// The draw loop first renders to this texture, then it is used for the wind
	// shader
	bool initScreenTexture();

	// Destroy resources associated to one or all entities created by the system
	~RenderSystem();

	// Draw all entities
	void draw(float elapsed_ms = 0.0f);

	
	vec4 getCameraView();
	
	mat3 createProjectionMatrix();

	void setCameraPosition(vec2 position) { 
		camera_position = position; 
	}
	vec2 getCameraPosition() const { return camera_position; }
	void resetInitialCameraPosition() { 
		// Only reset if not already initialized, or explicitly allow reset
		if (!camera_position_initialized) {
			initial_camera_position = camera_position; 
			camera_position_initialized = true;
		}
	}

	// toggle player hitbox debug rendering
	void togglePlayerHitboxDebug() { show_player_hitbox_debug = !show_player_hitbox_debug; }

private:
	// Internal drawing functions for each entity type
	void drawTexturedMesh(Entity entity, const mat3& projection);
	void drawIsocell(vec2 position, const mat3& projection);
	void drawChunks(const mat3 &projection);
	void drawToScreen();
	void drawEnemyHealthbar(Entity enemy_entity, const mat3& projection);
	void draw_particles();

	// Window handle
	GLFWwindow* window;

	// VAO
	GLuint vao;

	// Screen texture handles
	GLuint frame_buffer;
	GLuint off_screen_render_buffer_color;
	GLuint off_screen_render_buffer_depth;

	// SDF Shadow System Textures
	GLuint scene_fb, scene_texture;                    // Unlit scene render
	GLuint sdf_voronoi_fb1, sdf_voronoi_texture1;      // Jump flood ping-pong buffer 1
	GLuint sdf_voronoi_fb2, sdf_voronoi_texture2;      // Jump flood ping-pong buffer 2
	GLuint sdf_fb, sdf_texture;                        // Final signed distance field
	GLuint lighting_fb, lighting_texture;              // Composited lighting result

	// SDF Shadow System Shaders
	GLuint sdf_seed_program;              // Generates SDF seeds from scene objects
	GLuint sdf_jump_flood_program;        // Jump Flood Algorithm for Voronoi diagram
	GLuint sdf_distance_program;          // Converts Voronoi to distance field
	GLuint point_light_program;           // Renders lights with soft shadows using SDF

	GLuint particle_instance_vbo = 0;

	vec2 camera_position = {0.f, 0.f};
	vec2 initial_camera_position = {0.f, 0.f};
	bool camera_position_initialized = false;

	bool initShadowTextures();
	bool initShadowShaders();
	void renderLightingWithShadows();
	void renderSceneToColorTexture();

	Entity screen_state_entity;

	// debug flag for drawing player hitboxes
	bool show_player_hitbox_debug = false;
	
	// Low health overlay system
	LowHealthOverlaySystem* low_health_overlay_system = nullptr;
};

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program);

struct ParticleInstanceData {
    vec3 pos;
    float size;
    vec4 color;
};