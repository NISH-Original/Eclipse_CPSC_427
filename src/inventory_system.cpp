#include "inventory_system.hpp"
#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"
#include <iostream>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

InventorySystem::InventorySystem()
{
}

InventorySystem::~InventorySystem()
{
#ifdef HAVE_RMLUI
	if (inventory_document) {
		inventory_document->Close();
	}
	Rml::Shutdown();
#endif
}

bool InventorySystem::init(GLFWwindow* window)
{
#ifdef HAVE_RMLUI
	// Get framebuffer dimensions (accounts for Retina displays)
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);

	// Initialize RmlUi
	static RmlSystemInterface system_interface;
	static RmlRenderInterface render_interface;
	
	// Initialize render interface with shader
	if (!render_interface.init(width, height)) {
		std::cerr << "ERROR: Failed to initialize RmlRenderInterface" << std::endl;
		return false;
	}
	
	Rml::SetSystemInterface(&system_interface);
	Rml::SetRenderInterface(&render_interface);

	if (!Rml::Initialise()) {
		std::cerr << "ERROR: Failed to initialize RmlUi" << std::endl;
		return false;
	}
	
	// Load Press Start 2P font
	if (!Rml::LoadFontFace("data/fonts/PressStart2P-Regular.ttf")) {
		std::cerr << "WARNING: Failed to load Press Start 2P font" << std::endl;
	}

	// Create context
	rml_context = Rml::CreateContext("main", Rml::Vector2i(width, height));
	if (!rml_context) {
		std::cerr << "ERROR: Failed to create RmlUi context" << std::endl;
		return false;
	}

	// Create default items
	create_default_weapons();
	create_default_armors();
	
	return true;
#else
	std::cerr << "ERROR: RmlUi not available - inventory UI disabled (HAVE_RMLUI not defined)" << std::endl;
	return false;
#endif
}

void InventorySystem::create_default_weapons()
{
	// Create weapon entities with data matching the screenshot
	struct WeaponData {
		WeaponType type;
		std::string name;
		std::string description;
		int damage;
		int price;
		bool owned;
	};

	WeaponData weapon_data[] = {
		{WeaponType::LASER_PISTOL_GREEN, "Laser Pistol", "Base Pistol, reliable accurate.", 10, 0, true},
		{WeaponType::LASER_PISTOL_RED, "Laser Pistol", "Ceavy frame, increased power and recoil.", 15, 0, false},
		{WeaponType::PLASMA_SHOTGUN_HEAVY, "Plasma Shotgun", "Heavy frame, increased at close range.", 25, 0, false},
		{WeaponType::PLASMA_SHOTGUN_UNSTABLE, "Plasma Shotgun", "Unstable shotgun. Desanstanting at close range.", 30, 0, false},
		{WeaponType::SNIPER_RIFLE, "SniperRifle", "Crya blaster\nSnat pwosns roldclids.", 50, 500, false}
	};

	for (const auto& data : weapon_data) {
		Entity weapon_entity = Entity();
		Weapon& weapon = registry.weapons.emplace(weapon_entity);
		weapon.type = data.type;
		weapon.name = data.name;
		weapon.description = data.description;
		weapon.damage = data.damage;
		weapon.price = data.price;
		weapon.owned = data.owned;
		weapon.equipped = (data.type == WeaponType::LASER_PISTOL_GREEN && data.owned);
	}
}

void InventorySystem::create_default_armors()
{
	// Create some default armor pieces
	struct ArmorData {
		ArmorType type;
		std::string name;
		std::string description;
		int defense;
		int price;
		bool owned;
	};

	ArmorData armor_data[] = {
		{ArmorType::BASIC_SUIT, "Basic Suit", "Standard protection suit.", 5, 0, true},
		{ArmorType::ADVANCED_SUIT, "Advanced Suit", "Enhanced armor plating.", 15, 300, false},
		{ArmorType::HEAVY_SUIT, "Heavy Suit", "Maximum protection, reduced mobility.", 25, 600, false}
	};

	for (const auto& data : armor_data) {
		Entity armor_entity = Entity();
		Armor& armor = registry.armors.emplace(armor_entity);
		armor.type = data.type;
		armor.name = data.name;
		armor.description = data.description;
		armor.defense = data.defense;
		armor.price = data.price;
		armor.owned = data.owned;
		armor.equipped = (data.type == ArmorType::BASIC_SUIT && data.owned);
	}
}

void InventorySystem::init_player_inventory(Entity player_entity)
{
	if (!registry.players.has(player_entity)) {
		return;
	}

	// Create inventory component for player
	Inventory& inventory = registry.inventories.emplace(player_entity);
	
	// Add all weapons to inventory
	for (Entity weapon_entity : registry.weapons.entities) {
		inventory.weapons.push_back(weapon_entity);
		
		// Set equipped weapon
		Weapon& weapon = registry.weapons.get(weapon_entity);
		if (weapon.equipped) {
			inventory.equipped_weapon = weapon_entity;
		}
	}

	// Add all armors to inventory
	for (Entity armor_entity : registry.armors.entities) {
		inventory.armors.push_back(armor_entity);
		
		// Set equipped armor
		Armor& armor = registry.armors.get(armor_entity);
		if (armor.equipped) {
			inventory.equipped_armor = armor_entity;
		}
	}

	inventory.is_open = false;
}

void InventorySystem::update(float elapsed_ms)
{
#ifdef HAVE_RMLUI
	if (rml_context && inventory_open) {
		rml_context->Update();
		update_ui_data();
	}
#endif
}

void InventorySystem::render()
{
#ifdef HAVE_RMLUI
	if (rml_context && inventory_open) {
		// Save OpenGL state
		GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
		GLboolean cull_face = glIsEnabled(GL_CULL_FACE);
		GLboolean blend = glIsEnabled(GL_BLEND);
		GLint current_program;
		glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
		
		// Set up OpenGL state for 2D UI rendering
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		// Render the UI
		rml_context->Render();
		
		// Restore OpenGL state
		if (depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		if (cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
		glUseProgram(current_program);
	}
#endif
}

void InventorySystem::toggle_inventory()
{
	if (inventory_open) {
		hide_inventory();
	} else {
		show_inventory();
	}
}

void InventorySystem::show_inventory()
{
#ifdef HAVE_RMLUI
	if (!rml_context) {
		return;
	}

	if (!inventory_document) {
		inventory_document = rml_context->LoadDocument("ui/inventory.rml");
		if (!inventory_document) {
			std::cerr << "ERROR: Failed to load inventory.rml" << std::endl;
			return;
		}
	}

	inventory_document->Show();
	inventory_open = true;
	update_ui_data();
#endif
}

void InventorySystem::hide_inventory()
{
#ifdef HAVE_RMLUI
	if (inventory_document) {
		inventory_document->Hide();
	}
	inventory_open = false;
#endif
}

bool InventorySystem::is_inventory_open() const
{
	return inventory_open;
}

void InventorySystem::equip_weapon(Entity player_entity, Entity weapon_entity)
{
	if (!registry.players.has(player_entity) || !registry.inventories.has(player_entity)) {
		return;
	}

	if (!registry.weapons.has(weapon_entity)) {
		return;
	}

	Weapon& weapon = registry.weapons.get(weapon_entity);
	
	// Check if player owns this weapon
	if (!weapon.owned) {
		return;
	}

	Inventory& inventory = registry.inventories.get(player_entity);

	// Unequip current weapon
	if (registry.weapons.has(inventory.equipped_weapon)) {
		registry.weapons.get(inventory.equipped_weapon).equipped = false;
	}

	// Equip new weapon
	weapon.equipped = true;
	inventory.equipped_weapon = weapon_entity;

	update_ui_data();
}

void InventorySystem::equip_armor(Entity player_entity, Entity armor_entity)
{
	if (!registry.players.has(player_entity) || !registry.inventories.has(player_entity)) {
		return;
	}

	if (!registry.armors.has(armor_entity)) {
		return;
	}

	Armor& armor = registry.armors.get(armor_entity);
	
	// Check if player owns this armor
	if (!armor.owned) {
		return;
	}

	Inventory& inventory = registry.inventories.get(player_entity);

	// Unequip current armor
	if (registry.armors.has(inventory.equipped_armor)) {
		registry.armors.get(inventory.equipped_armor).equipped = false;
	}

	// Equip new armor
	armor.equipped = true;
	inventory.equipped_armor = armor_entity;

	update_ui_data();
}

bool InventorySystem::buy_item(Entity player_entity, Entity item_entity)
{
	if (!registry.players.has(player_entity)) {
		return false;
	}

	Player& player = registry.players.get(player_entity);

	// Try to buy weapon
	if (registry.weapons.has(item_entity)) {
		Weapon& weapon = registry.weapons.get(item_entity);
		
		if (weapon.owned) {
			return false; // Already owned
		}

		if (player.currency >= weapon.price) {
			player.currency -= weapon.price;
			weapon.owned = true;
			update_ui_data();
			return true;
		}
		return false;
	}

	// Try to buy armor
	if (registry.armors.has(item_entity)) {
		Armor& armor = registry.armors.get(item_entity);
		
		if (armor.owned) {
			return false; // Already owned
		}

		if (player.currency >= armor.price) {
			player.currency -= armor.price;
			armor.owned = true;
			update_ui_data();
			return true;
		}
		return false;
	}

	return false;
}

void InventorySystem::update_ui_data()
{
#ifdef HAVE_RMLUI
	if (!inventory_document || !rml_context) {
		return;
	}

	if (registry.players.entities.empty()) {
		return;
	}

	Entity player_entity = registry.players.entities[0];
	Player& player = registry.players.get(player_entity);

	// Update currency display
	if (Rml::Element* currency_text = inventory_document->GetElementById("currency_text")) {
		currency_text->SetInnerRML(std::to_string(player.currency));
	}
#endif
}

#ifdef HAVE_RMLUI
// RmlSystemInterface implementation
double RmlSystemInterface::GetElapsedTime()
{
	return glfwGetTime();
}

// RmlRenderInterface implementation
RmlRenderInterface::RmlRenderInterface() 
	: shader_program(0), width(0), height(0), dummy_texture(0), current_transform(Rml::Matrix4f::Identity())
{
}

RmlRenderInterface::~RmlRenderInterface()
{
	if (shader_program) {
		glDeleteProgram(shader_program);
	}
	if (dummy_texture) {
		glDeleteTextures(1, &dummy_texture);
	}
}

bool RmlRenderInterface::init(int window_width, int window_height)
{
	width = window_width;
	height = window_height;
	
	// Load UI shader
	std::string vs_path = shader_path("ui") + ".vs.glsl";
	std::string fs_path = shader_path("ui") + ".fs.glsl";
	
	if (!loadEffectFromFile(vs_path, fs_path, shader_program)) {
		std::cerr << "Failed to load UI shader" << std::endl;
		return false;
	}
	
	// Get uniform locations
	projection_uniform = glGetUniformLocation(shader_program, "projection");
	translation_uniform = glGetUniformLocation(shader_program, "translation");
	use_texture_uniform = glGetUniformLocation(shader_program, "use_texture");
	GLint tex_uniform = glGetUniformLocation(shader_program, "tex");
	
	// Set the texture sampler to use texture unit 0
	glUseProgram(shader_program);
	if (tex_uniform != -1) {
		glUniform1i(tex_uniform, 0);
	}
	glUseProgram(0);
	
	// Create a 1x1 white dummy texture for non-textured rendering
	unsigned char white_pixel[4] = {255, 255, 255, 255};
	glGenTextures(1, &dummy_texture);
	glBindTexture(GL_TEXTURE_2D, dummy_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white_pixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	return true;
}

Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices,
                                                                 Rml::Span<const int> indices)
{
	CompiledGeometry* geom = new CompiledGeometry();
	
	glGenVertexArrays(1, &geom->vao);
	glGenBuffers(1, &geom->vbo);
	glGenBuffers(1, &geom->ibo);
	
	glBindVertexArray(geom->vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, geom->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Rml::Vertex), vertices.data(), GL_STATIC_DRAW);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom->ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), 
	                      (void*)offsetof(Rml::Vertex, position));
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Rml::Vertex), 
	                      (void*)offsetof(Rml::Vertex, colour));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Rml::Vertex), 
	                      (void*)offsetof(Rml::Vertex, tex_coord));
	
	glBindVertexArray(0);
	
	geom->num_indices = indices.size();
	geometries.push_back(geom);
	
	return (Rml::CompiledGeometryHandle)geom;
}

void RmlRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry,
                                        Rml::Vector2f translation,
                                        Rml::TextureHandle texture)
{
	CompiledGeometry* geom = (CompiledGeometry*)geometry;
	if (!geom || !shader_program) return;
	
	// Use our shader program
	glUseProgram(shader_program);
	
	// Check if uniforms are valid
	if (projection_uniform == -1 || translation_uniform == -1 || use_texture_uniform == -1) {
		std::cerr << "Invalid uniform locations: proj=" << projection_uniform 
		          << " trans=" << translation_uniform 
		          << " tex=" << use_texture_uniform << std::endl;
		return;
	}
	
	// Set up orthographic projection matrix (top-left origin, like RmlUi expects)
	float projection_matrix[16] = {
		2.0f / width, 0.0f,            0.0f, 0.0f,
		0.0f,         -2.0f / height,  0.0f, 0.0f,
		0.0f,         0.0f,            1.0f, 0.0f,
		-1.0f,        1.0f,            0.0f, 1.0f
	};
	
	glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, projection_matrix);
	glUniform2f(translation_uniform, translation.x, translation.y);
	
	// Set texture (use dummy texture for non-textured rendering to avoid warnings)
	bool has_texture = (texture != 0);
	glUniform1i(use_texture_uniform, has_texture ? 1 : 0);
	
	glActiveTexture(GL_TEXTURE0);
	if (has_texture) {
		glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
	} else {
		glBindTexture(GL_TEXTURE_2D, dummy_texture);
	}
	
	// Render the geometry
	glBindVertexArray(geom->vao);
	
	glDrawElements(GL_TRIANGLES, geom->num_indices, GL_UNSIGNED_INT, nullptr);
	
	glBindVertexArray(0);
	
	// Unbind shader and texture
	glUseProgram(0);
	if (has_texture) {
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	
	while (glGetError() != GL_NO_ERROR);
}

void RmlRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	CompiledGeometry* geom = (CompiledGeometry*)geometry;
	if (!geom) return;
	
	glDeleteVertexArrays(1, &geom->vao);
	glDeleteBuffers(1, &geom->vbo);
	glDeleteBuffers(1, &geom->ibo);
	
	auto it = std::find(geometries.begin(), geometries.end(), geom);
	if (it != geometries.end()) {
		geometries.erase(it);
	}
	
	delete geom;
}

void RmlRenderInterface::EnableScissorRegion(bool enable)
{
	if (enable) {
		glEnable(GL_SCISSOR_TEST);
	} else {
		glDisable(GL_SCISSOR_TEST);
	}
}

void RmlRenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
	// OpenGL uses bottom-left origin, RmlUi uses top-left
	// We need to flip the Y coordinate
	int gl_y = height - (region.Top() + region.Height());
	glScissor(region.Left(), gl_y, region.Width(), region.Height());
}

Rml::TextureHandle RmlRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions,
                                                     const Rml::String& source)
{
	// Not implemented - return 0
	return 0;
}

Rml::TextureHandle RmlRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                         Rml::Vector2i source_dimensions)
{
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, source_dimensions.x, source_dimensions.y,
	             0, GL_RGBA, GL_UNSIGNED_BYTE, source.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	return (Rml::TextureHandle)texture_id;
}

void RmlRenderInterface::ReleaseTexture(Rml::TextureHandle texture_handle)
{
	GLuint texture_id = (GLuint)texture_handle;
	glDeleteTextures(1, &texture_id);
}

void RmlRenderInterface::SetTransform(const Rml::Matrix4f* transform)
{
	if (transform) {
		current_transform = *transform;
	} else {
		current_transform = Rml::Matrix4f::Identity();
	}
}
#endif

