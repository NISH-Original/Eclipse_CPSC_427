#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

#include <string>
#include <functional>

class InventorySystem
#ifdef HAVE_RMLUI
	: public Rml::EventListener
#endif
{
public:
	InventorySystem();
	~InventorySystem();

	// Initialize the inventory system with RmlUi context
	bool init(GLFWwindow* window);

	// Update the inventory UI
	void update(float elapsed_ms);

	// Render the inventory UI
	void render();

	// Toggle inventory visibility
	void toggle_inventory();

	// Show/hide inventory
	void show_inventory();
	void hide_inventory();

	// Check if inventory is open
	bool is_inventory_open() const;
	
	// Input event handling
	void on_mouse_move(vec2 mouse_position);
	void on_mouse_button(int button, int action, int mods);
	
	// Set the window handle for cursor management
	void set_window(GLFWwindow* window);

	// Initialize player inventory with default items
	void init_player_inventory(Entity player_entity);

	// Equip a weapon
	void equip_weapon(Entity player_entity, Entity weapon_entity);

	// Equip armor
	void equip_armor(Entity player_entity, Entity armor_entity);

	// Buy an item
	bool buy_item(Entity player_entity, Entity item_entity);

	// Update UI data bindings
	void update_ui_data();

#ifdef HAVE_RMLUI
	// Override EventListener method
	void ProcessEvent(Rml::Event& event) override;
#endif

	// Reload UI files (for hot reloading during development)
	void reload_ui();
	void reload_stylesheet_only();

	// Get RmlUi context (for sharing with other systems like HUD)
	Rml::Context* get_context() const;
	
	// Set callback for when inventory is closed
	void set_on_close_callback(std::function<void()> callback);

	void create_default_weapons();
	void create_default_armors();

private:
	bool inventory_open = false;
	GLFWwindow* window = nullptr;
	GLFWcursor* hand_cursor = nullptr;
	bool is_hovering_button = false;
	vec2 last_mouse_position = {0, 0};
	
	// Callback function called when inventory is closed
	std::function<void()> on_close_callback = nullptr;

#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* inventory_document = nullptr;
	
	// File modification tracking for hot reload
	time_t last_rml_mod_time = 0;
	time_t last_rcss_mod_time = 0;
#endif
	
	// Helper to check if mouse is over a button
	bool is_mouse_over_button(vec2 mouse_position);

	time_t get_file_mod_time(const std::string& filepath);
};

#ifdef HAVE_RMLUI
// Custom RmlUi System Interface for GLFW
class RmlSystemInterface : public Rml::SystemInterface
{
public:
	double GetElapsedTime() override;
	bool LogMessage(Rml::Log::Type type, const Rml::String& message) override;
};

// Custom RmlUi Render Interface for OpenGL
class RmlRenderInterface : public Rml::RenderInterface
{
public:
	RmlRenderInterface();
	~RmlRenderInterface();
	
	bool init(int window_width, int window_height);
	
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
	                                             Rml::Span<const int> indices) override;

	void RenderGeometry(Rml::CompiledGeometryHandle geometry,
	                    Rml::Vector2f translation,
	                    Rml::TextureHandle texture) override;

	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	void EnableScissorRegion(bool enable) override;
	void SetScissorRegion(Rml::Rectanglei region) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions,
	                                const Rml::String& source) override;

	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source,
	                                    Rml::Vector2i source_dimensions) override;

	void ReleaseTexture(Rml::TextureHandle texture_handle) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

private:
	struct CompiledGeometry {
		GLuint vao, vbo, ibo;
		int num_indices;
	};
	
	std::vector<CompiledGeometry*> geometries;
	
	GLuint shader_program;
	GLint projection_uniform;
	GLint translation_uniform;
	GLint use_texture_uniform;
	
	int width;
	int height;
	
	GLuint dummy_texture; 
	
	Rml::Matrix4f current_transform;
};
#endif

