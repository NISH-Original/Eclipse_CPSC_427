#pragma once

#include "common.hpp"
#include "tiny_ecs.hpp"
#include "components.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

#include <string>

class InventorySystem
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
	void process_event(Rml::Event& event);
#endif

private:
	bool inventory_open = false;

#ifdef HAVE_RMLUI
	Rml::Context* rml_context = nullptr;
	Rml::ElementDocument* inventory_document = nullptr;
#endif

	void create_default_weapons();
	void create_default_armors();
};

#ifdef HAVE_RMLUI
// Custom RmlUi System Interface for GLFW
class RmlSystemInterface : public Rml::SystemInterface
{
public:
	double GetElapsedTime() override;
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

