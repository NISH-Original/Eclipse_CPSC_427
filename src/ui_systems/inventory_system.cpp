#include "inventory_system.hpp"
#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"
#include "audio_system.hpp"
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <chrono>
#include <unordered_map>

#include <stb_image.h>

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

InventorySystem::InventorySystem()
{
}

InventorySystem::~InventorySystem()
{
#ifdef HAVE_RMLUI
	// Note: Don't call inventory_document->Close() here to avoid crash during RmlUI shutdown
	// RmlUI will clean up documents when Rml::Shutdown() is called in main.cpp
	// Note: Rml::Shutdown() moved to main.cpp to avoid race condition with other UI systems
#endif
	
	if (hand_cursor) {
		glfwDestroyCursor(hand_cursor);
		hand_cursor = nullptr;
	}
}

bool InventorySystem::init(GLFWwindow* window)
{
	this->window = window;
#ifdef HAVE_RMLUI
	// NOTE: old system left for reference
	//int width, height;
	//glfwGetFramebufferSize(window, &width, &height);
	//int inv_width = (width == window_width_px) ? width*2 : width;
	//int inv_height = (height == window_height_px) ? height*2 : height;

	// TODO: apply nishant's hotfix
	int width = window_width_px * 2;
	int height = window_height_px * 2; 

	static RmlSystemInterface system_interface;
	static RmlRenderInterface render_interface;
	
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
	
	if (!Rml::LoadFontFace("data/fonts/PressStart2P-Regular.ttf")) {
		std::cerr << "WARNING: Failed to load Press Start 2P font" << std::endl;
	}

	rml_context = Rml::CreateContext("main", Rml::Vector2i(width, height));
	if (!rml_context) {
		std::cerr << "ERROR: Failed to create RmlUi context" << std::endl;
		return false;
	}

	create_default_weapons();
	create_default_armours();
	
	return true;
#else
	std::cerr << "ERROR: RmlUi not available - inventory UI disabled (HAVE_RMLUI not defined)" << std::endl;
	return false;
#endif
}

Rml::Context* InventorySystem::get_context() const
{
#ifdef HAVE_RMLUI
	return rml_context;
#else
	return nullptr;
#endif
}

void InventorySystem::set_on_close_callback(std::function<void(bool)> callback)
{
	on_close_callback = callback;
}

void InventorySystem::set_on_next_level_callback(std::function<void()> callback)
{
	on_next_level_callback = callback;
}

void InventorySystem::set_on_weapon_equip_callback(std::function<void()> callback)
{
	on_weapon_equip_callback = callback;
}

void InventorySystem::set_audio_system(AudioSystem* audio)
{
	audio_system = audio;
}

void InventorySystem::create_default_weapons()
{
	struct WeaponData {
		WeaponType type;
		std::string name;
		std::string description;
		int damage;
		int price;
		bool owned;
	};

	WeaponData weapon_data[] = {
		{WeaponType::LASER_PISTOL_GREEN, "Laser Pistol", "Base Pistol, reliable accurate.", 20, 0, true},
		{WeaponType::EXPLOSIVE_RIFLE, "Explosive Rifle", "Rifle rounds explode on impact, damaging nearby foes.", 50, 500, false},
		{WeaponType::PLASMA_SHOTGUN_HEAVY, "Plasma Shotgun", "Heavy frame, increased at close range.", 25, 500, false},
		{WeaponType::ASSAULT_RIFLE, "Assault Rifle", "Rapid-fire automatic weapon.", 20, 500, false}
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
		
		
		if (weapon.type == WeaponType::ASSAULT_RIFLE) {
			weapon.fire_rate_rpm = 600.0f; // 600 rounds per minute
		}
		
		registry.weaponUpgrades.emplace(weapon_entity);
	}
}

void InventorySystem::create_default_armours()
{
	struct armourData {
		armourType type;
		std::string name;
		std::string description;
		int defense;
		int price;
		bool owned;
	};

	armourData armour_data[] = {
		{armourType::BASIC_SUIT, "Basic Suit", "Standard protection suit.", 5, 0, true},
		{armourType::ADVANCED_SUIT, "Advanced Suit", "Enhanced armour plating.", 15, 300, false},
		{armourType::HEAVY_SUIT, "Heavy Suit", "Maximum protection, reduced mobility.", 25, 600, false}
	};

	for (const auto& data : armour_data) {
		Entity armour_entity = Entity();
		armour& armour = registry.armours.emplace(armour_entity);
		armour.type = data.type;
		armour.name = data.name;
		armour.description = data.description;
		armour.defense = data.defense;
		armour.price = data.price;
		armour.owned = data.owned;
		armour.equipped = (data.type == armourType::BASIC_SUIT && data.owned);
	}
}

void InventorySystem::init_player_inventory(Entity player_entity)
{
	if (!registry.players.has(player_entity)) {
		return;
	}

	Inventory& inventory = registry.inventories.emplace(player_entity);
	
	for (Entity weapon_entity : registry.weapons.entities) {
		inventory.weapons.push_back(weapon_entity);
		
		Weapon& weapon = registry.weapons.get(weapon_entity);
		if (weapon.equipped) {
			inventory.equipped_weapon = weapon_entity;
		}
	}

	for (Entity armour_entity : registry.armours.entities) {
		inventory.armours.push_back(armour_entity);
		
		armour& armour = registry.armours.get(armour_entity);
		if (armour.equipped) {
			inventory.equipped_armour = armour_entity;
		}
	}

	inventory.is_open = false;
}

void InventorySystem::update(float elapsed_ms)
{
#ifdef HAVE_RMLUI
	if (rml_context) {
		rml_context->Update();
	}
	
	if (rml_context && inventory_open) {
		time_t rml_mod = max(get_file_mod_time("../ui/inventory.rml"), get_file_mod_time("ui/inventory.rml"));
		time_t rcss_mod = max(get_file_mod_time("../ui/inventory.rcss"), get_file_mod_time("ui/inventory.rcss"));
		
		bool rml_changed = (rml_mod != last_rml_mod_time && last_rml_mod_time != 0);
		bool rcss_changed = (rcss_mod != last_rcss_mod_time && last_rcss_mod_time != 0);
		
		if (rml_changed || rcss_changed) {
			if (rml_changed) {
				reload_ui();
			} else if (rcss_changed) {
				reload_stylesheet_only();
			}
		}
		
		last_rml_mod_time = rml_mod;
		last_rcss_mod_time = rcss_mod;
	}
#endif
}

void InventorySystem::render()
{
#ifdef HAVE_RMLUI
	if (rml_context && window) {
		GLboolean depth_test = glIsEnabled(GL_DEPTH_TEST);
		GLboolean cull_face = glIsEnabled(GL_CULL_FACE);
		GLboolean blend = glIsEnabled(GL_BLEND);
		GLboolean scissor_test = glIsEnabled(GL_SCISSOR_TEST);
		GLint current_program;
		GLint viewport[4];
		glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
		glGetIntegerv(GL_VIEWPORT, viewport);
		
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		
		rml_context->Render();
		
		if (depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		if (cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
		if (blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
		if (scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
		glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
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
		std::cerr << "ERROR: rml_context is null!" << std::endl;
		return;
	}

	if (!inventory_document) {
		inventory_document = rml_context->LoadDocument("../ui/inventory.rml");
		if (!inventory_document) {
			// try alternate filepath
			inventory_document = rml_context->LoadDocument("ui/inventory.rml");
			if (!inventory_document) {
				std::cerr << "ERROR: Failed to load inventory.rml" << std::endl;
				return;
			}
		}
		
		if (Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab")) {
			weapons_tab->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* upgrades_tab = inventory_document->GetElementById("upgrades_tab")) {
			upgrades_tab->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* close_btn = inventory_document->GetElementById("close_inventory_btn")) {
			close_btn->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* next_level_btn = inventory_document->GetElementById("next_level_btn")) {
			next_level_btn->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* weapon_list = inventory_document->GetElementById("weapon_list")) {
			weapon_list->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* player_upgrade_list = inventory_document->GetElementById("player_upgrade_list")) {
			player_upgrade_list->AddEventListener(Rml::EventId::Click, this);
		}
	}
	
	inventory_open = true;
	
	// Set cursor to default Windows cursor when inventory opens
	if (window) {
		glfwSetCursor(window, nullptr);
		is_hovering_button = false;
	}
	
	last_rml_mod_time = max(get_file_mod_time("../ui/inventory.rml"), get_file_mod_time("ui/inventory.rml"));
	last_rcss_mod_time = max(get_file_mod_time("../ui/inventory.rcss"), get_file_mod_time("ui/inventory.rcss"));
	
	update_ui_data();
	inventory_document->Show();
#endif
}

void InventorySystem::hide_inventory()
{
#ifdef HAVE_RMLUI
	if (inventory_document) {
		inventory_document->Hide();
	}
	inventory_open = false;
	
	if (window && default_cursor) {
		glfwSetCursor(window, default_cursor);
	}
	is_hovering_button = false;
#endif
}

bool InventorySystem::is_inventory_open() const
{
	return inventory_open;
}

void InventorySystem::set_window(GLFWwindow* window_ptr)
{
	window = window_ptr;
	if (window) {
		hand_cursor = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
	}
}

void InventorySystem::set_default_cursor(GLFWcursor* cursor)
{
	default_cursor = cursor;
}

void InventorySystem::on_mouse_move(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (rml_context && inventory_open) {
		rml_context->ProcessMouseMove((int)(mouse_position.x * 2), (int)(mouse_position.y * 2), 0);
		last_mouse_position = mouse_position;
		
		if (window && hand_cursor) {
			bool should_show_hand = is_mouse_over_button(mouse_position);
			
			if (should_show_hand && !is_hovering_button) {
				glfwSetCursor(window, hand_cursor);
				is_hovering_button = true;
			} else if (!should_show_hand && is_hovering_button) {
				glfwSetCursor(window, nullptr);
				is_hovering_button = false;
			}
		}
	}
#endif
}

void InventorySystem::on_mouse_button(int button, int action, int mods)
{
#ifdef HAVE_RMLUI
	if (rml_context && inventory_open) {
		int rml_button = button;
		
		if (action == GLFW_PRESS) {
			rml_context->ProcessMouseButtonDown(rml_button, 0);
		} else if (action == GLFW_RELEASE) {
			rml_context->ProcessMouseButtonUp(rml_button, 0);
		}
	}
#endif
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
	
	if (!weapon.owned) {
		return;
	}

	Inventory& inventory = registry.inventories.get(player_entity);

	if (registry.weapons.has(inventory.equipped_weapon)) {
		registry.weapons.get(inventory.equipped_weapon).equipped = false;
	}

	weapon.equipped = true;
	inventory.equipped_weapon = weapon_entity;

	if (registry.players.has(player_entity)) {
		Player& player = registry.players.get(player_entity);
		
		if (!registry.weaponUpgrades.has(weapon_entity)) {
			registry.weaponUpgrades.emplace(weapon_entity);
		}
		WeaponUpgrades& upgrades = registry.weaponUpgrades.get(weapon_entity);
		
		int base_magazine_size;
		if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
			base_magazine_size = 5;
		} else if (weapon.type == WeaponType::ASSAULT_RIFLE) {
			base_magazine_size = 30;
		} else if (weapon.type == WeaponType::EXPLOSIVE_RIFLE) {
			player.magazine_size = 1;
		} else {
			base_magazine_size = 10;
		}
		
		player.magazine_size = base_magazine_size + (upgrades.ammo_capacity_level * WeaponUpgrades::AMMO_PER_LEVEL);
		player.ammo_in_mag = player.magazine_size;
	}

	if (registry.sprites.has(player_entity) && registry.renderRequests.has(player_entity)) {
		Sprite& sprite = registry.sprites.get(player_entity);
		
		if (registry.weaponUpgrades.has(weapon_entity)) {
			WeaponUpgrades& upgrades = registry.weaponUpgrades.get(weapon_entity);
			float base_reload_duration = 1.5f;
			float reload_multiplier = 1.0f;
			for (int i = 0; i < upgrades.reload_time_level; i++) {
				reload_multiplier *= (1.0f - WeaponUpgrades::RELOAD_TIME_REDUCTION_PER_LEVEL);
			}
			sprite.reload_duration = base_reload_duration * reload_multiplier;
		} else {
			sprite.reload_duration = 1.5f;
		}
		auto& render_request = registry.renderRequests.get(player_entity);

		sprite.is_reloading = false;
		sprite.is_shooting = false;
		sprite.curr_frame = 0;
		sprite.step_seconds_acc = 0.0f;

		// determine texture based on weapon type
		TEXTURE_ASSET_ID base_texture;
		if (sprite.current_animation == TEXTURE_ASSET_ID::PLAYER_MOVE) {
			base_texture = TEXTURE_ASSET_ID::PLAYER_MOVE;
		} else {
			base_texture = TEXTURE_ASSET_ID::PLAYER_IDLE;
		}
		
		if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
			if (base_texture == TEXTURE_ASSET_ID::PLAYER_IDLE) {
				render_request.used_texture = TEXTURE_ASSET_ID::SHOTGUN_IDLE;
				sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_MOVE) {
				render_request.used_texture = TEXTURE_ASSET_ID::SHOTGUN_MOVE;
				sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_MOVE;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_SHOOT) {
				render_request.used_texture = TEXTURE_ASSET_ID::SHOTGUN_SHOOT;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_RELOAD) {
				render_request.used_texture = TEXTURE_ASSET_ID::SHOTGUN_RELOAD;
			}
		} else if (weapon.type == WeaponType::ASSAULT_RIFLE || weapon.type == WeaponType::EXPLOSIVE_RIFLE) {
			if (base_texture == TEXTURE_ASSET_ID::PLAYER_IDLE) {
				render_request.used_texture = TEXTURE_ASSET_ID::RIFLE_IDLE;
				sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_MOVE) {
				render_request.used_texture = TEXTURE_ASSET_ID::RIFLE_MOVE;
				sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_MOVE;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_SHOOT) {
				render_request.used_texture = TEXTURE_ASSET_ID::RIFLE_SHOOT;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_RELOAD) {
				render_request.used_texture = TEXTURE_ASSET_ID::RIFLE_RELOAD;
			}
		} else {
			if (base_texture == TEXTURE_ASSET_ID::PLAYER_IDLE) {
				render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_IDLE;
				sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_MOVE) {
				render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_MOVE;
				sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_MOVE;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_SHOOT) {
				render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_SHOOT;
			} else if (base_texture == TEXTURE_ASSET_ID::PLAYER_RELOAD) {
				render_request.used_texture = TEXTURE_ASSET_ID::PLAYER_RELOAD;
			}
		}
	}

	update_ui_data();
	
	// Notify that weapon was equipped (for cursor update)
	if (on_weapon_equip_callback) {
		on_weapon_equip_callback();
	}
}

void InventorySystem::equip_armour(Entity player_entity, Entity armour_entity)
{
	if (!registry.players.has(player_entity) || !registry.inventories.has(player_entity)) {
		return;
	}

	if (!registry.armours.has(armour_entity)) {
		return;
	}

	armour& armour = registry.armours.get(armour_entity);
	
	if (!armour.owned) {
		return;
	}

	Inventory& inventory = registry.inventories.get(player_entity);

	if (registry.armours.has(inventory.equipped_armour)) {
		registry.armours.get(inventory.equipped_armour).equipped = false;
	}

	armour.equipped = true;
	inventory.equipped_armour = armour_entity;

	update_ui_data();
}

bool InventorySystem::buy_item(Entity player_entity, Entity item_entity)
{
	if (!registry.players.has(player_entity)) {
		return false;
	}

	Player& player = registry.players.get(player_entity);

	if (registry.weapons.has(item_entity)) {
		Weapon& weapon = registry.weapons.get(item_entity);
		
		if (weapon.owned) {
			return false;
		}

		if (player.currency >= weapon.price) {
			player.currency -= weapon.price;
			weapon.owned = true;
			update_ui_data();
			// Play xylarite spend sound
			if (audio_system) {
				audio_system->play("xylarite_spend");
			}
			return true;
		}
		return false;
	}

	if (registry.armours.has(item_entity)) {
		armour& armour = registry.armours.get(item_entity);
		
		if (armour.owned) {
			return false;
		}

		if (player.currency >= armour.price) {
			player.currency -= armour.price;
			armour.owned = true;
			update_ui_data();
			// Play xylarite spend sound
			if (audio_system) {
				audio_system->play("xylarite_spend");
			}
			return true;
		}
		return false;
	}

	return false;
}

// Handles purchasing upgrades from the upgrade menu
bool InventorySystem::buy_upgrade(Entity player_entity, const std::string& upgrade_type)
{
	if (!registry.players.has(player_entity)) {
		return false;
	}

	Player& player = registry.players.get(player_entity);

	if (!registry.playerUpgrades.has(player_entity)) {
		registry.playerUpgrades.emplace(player_entity);
	}

	PlayerUpgrades& upgrades = registry.playerUpgrades.get(player_entity);

	// Struct to hold the cost and a pointer to the upgrade level variable
	struct UpgradeData {
		int cost;
		int PlayerUpgrades::* level_ptr;
	};

	// Map of upgrade type strings to their cost and level pointer
	// Using member pointers so we can increment the right level variable
	static const std::unordered_map<std::string, UpgradeData> upgrade_map = {
		{"movement_speed",     {PlayerUpgrades::MOVEMENT_SPEED_COST,   &PlayerUpgrades::movement_speed_level}},
		{"max_health",         {PlayerUpgrades::MAX_HEALTH_COST,       &PlayerUpgrades::max_health_level}},
		{"armour",             {PlayerUpgrades::ARMOUR_COST,           &PlayerUpgrades::armour_level}},
		{"light_radius",       {PlayerUpgrades::LIGHT_RADIUS_COST,     &PlayerUpgrades::light_radius_level}},
		{"dash_cooldown",      {PlayerUpgrades::DASH_COOLDOWN_COST,    &PlayerUpgrades::dash_cooldown_level}},
		{"health_regen",       {PlayerUpgrades::HEALTH_REGEN_COST,     &PlayerUpgrades::health_regen_level}},
		{"crit_chance",        {PlayerUpgrades::CRIT_CHANCE_COST,      &PlayerUpgrades::crit_chance_level}},
		{"life_steal",         {PlayerUpgrades::LIFE_STEAL_COST,       &PlayerUpgrades::life_steal_level}},
		{"flashlight_width",   {PlayerUpgrades::FLASHLIGHT_WIDTH_COST, &PlayerUpgrades::flashlight_width_level}},
		{"flashlight_damage",  {PlayerUpgrades::FLASHLIGHT_DAMAGE_COST,&PlayerUpgrades::flashlight_damage_level}},
		{"flashlight_slow",    {PlayerUpgrades::FLASHLIGHT_SLOW_COST,  &PlayerUpgrades::flashlight_slow_level}},
		{"xylarite_multiplier",{PlayerUpgrades::XYLARITE_MULTIPLIER_COST, &PlayerUpgrades::xylarite_multiplier_level}},
	};

	// Find the upgrade in the map
	auto it = upgrade_map.find(upgrade_type);
	if (it == upgrade_map.end()) {
		return false;
	}

	const UpgradeData& data = it->second;
	// Use member pointer to get reference to the actual level variable
	int& level = upgrades.*(data.level_ptr);

	// Check if already maxed
	if (level >= PlayerUpgrades::MAX_UPGRADE_LEVEL) {
		return false;
	}

	// Check if player can afford it
	if (player.currency < data.cost) {
		return false;
	}

	// Deduct cost and increase level
	player.currency -= data.cost;
	level++;

	// Some upgrades need to update player stats immediately
	if (upgrade_type == "movement_speed") {
		player.speed = 200.0f + (upgrades.movement_speed_level * PlayerUpgrades::MOVEMENT_SPEED_PER_LEVEL);
	}
	else if (upgrade_type == "max_health") {
		player.max_health = 100 + (upgrades.max_health_level * PlayerUpgrades::HEALTH_PER_LEVEL);
		if (player.health > player.max_health) {
			player.health = player.max_health;
		}
	}
	else if (upgrade_type == "armour") {
		player.max_armour = upgrades.armour_level * PlayerUpgrades::ARMOUR_PER_LEVEL;
	}

	update_ui_data();
	
	// Play xylarite spend sound
	if (audio_system) {
		audio_system->play("xylarite_spend");
	}
	
	return true;
}

// Handles purchasing weapon upgrades (fire rate and damage)
bool InventorySystem::buy_weapon_upgrade(Entity player_entity, Entity weapon_entity, const std::string& upgrade_type)
{
	if (!registry.players.has(player_entity) || !registry.weapons.has(weapon_entity)) {
		return false;
	}

	Player& player = registry.players.get(player_entity);
	Weapon& weapon = registry.weapons.get(weapon_entity);
	
	if (!weapon.owned) {
		return false;
	}

	if (!registry.weaponUpgrades.has(weapon_entity)) {
		registry.weaponUpgrades.emplace(weapon_entity);
	}

	WeaponUpgrades& upgrades = registry.weaponUpgrades.get(weapon_entity);

	struct UpgradeData {
		int cost;
		int WeaponUpgrades::* level_ptr;
	};

	static const std::unordered_map<std::string, UpgradeData> upgrade_map = {
		{"weapon_damage",       {WeaponUpgrades::DAMAGE_COST,       &WeaponUpgrades::damage_level}},
		{"weapon_magazine_size", {WeaponUpgrades::AMMO_CAPACITY_COST, &WeaponUpgrades::ammo_capacity_level}},
		{"weapon_reload_time",  {WeaponUpgrades::RELOAD_TIME_COST,  &WeaponUpgrades::reload_time_level}},
	};

	auto it = upgrade_map.find(upgrade_type);
	if (it == upgrade_map.end()) {
		return false;
	}

	const UpgradeData& data = it->second;
	int& level = upgrades.*(data.level_ptr);

	if (level >= WeaponUpgrades::MAX_UPGRADE_LEVEL) {
		return false;
	}

	if (player.currency < data.cost) {
		return false;
	}

	player.currency -= data.cost;
	level++;

	if (upgrade_type == "weapon_magazine_size" && weapon.equipped) {
		int base_magazine_size;
		if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
			base_magazine_size = 5;
		} else if (weapon.type == WeaponType::ASSAULT_RIFLE) {
			base_magazine_size = 30;
		} else {
			base_magazine_size = 10;
		}
		
		player.magazine_size = base_magazine_size + (upgrades.ammo_capacity_level * WeaponUpgrades::AMMO_PER_LEVEL);
		player.ammo_in_mag = player.magazine_size;
	}
	
	if (upgrade_type == "weapon_reload_time" && weapon.equipped && registry.sprites.has(player_entity)) {
		Sprite& sprite = registry.sprites.get(player_entity);
		float base_reload_duration = 1.5f;
		float reload_multiplier = 1.0f;
		for (int i = 0; i < upgrades.reload_time_level; i++) {
			reload_multiplier *= (1.0f - WeaponUpgrades::RELOAD_TIME_REDUCTION_PER_LEVEL);
		}
		sprite.reload_duration = base_reload_duration * reload_multiplier;
	}

	update_ui_data();
	return true;
}

void InventorySystem::reload_ui()
{
#ifdef HAVE_RMLUI
	if (!rml_context) {
		return;
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	
	Rml::Factory::ClearStyleSheetCache();
	Rml::Factory::ClearTemplateCache();
	
	if (inventory_document) {
		inventory_document->Close();
		inventory_document = nullptr;
	}
	
	inventory_document = rml_context->LoadDocument("ui/inventory.rml");
	if (!inventory_document) {
		std::cerr << "ERROR: Failed to reload inventory.rml" << std::endl;
		return;
	}
	
	if (Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab")) {
		weapons_tab->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* upgrades_tab = inventory_document->GetElementById("upgrades_tab")) {
		upgrades_tab->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* close_btn = inventory_document->GetElementById("close_inventory_btn")) {
		close_btn->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* next_level_btn = inventory_document->GetElementById("next_level_btn")) {
		next_level_btn->AddEventListener(Rml::EventId::Click, this);
	}
	
	if (Rml::Element* weapon_list = inventory_document->GetElementById("weapon_list")) {
		weapon_list->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* player_upgrade_list = inventory_document->GetElementById("player_upgrade_list")) {
		player_upgrade_list->AddEventListener(Rml::EventId::Click, this);
	}

	inventory_document->Show();
	update_ui_data();
#endif
}

void InventorySystem::reload_stylesheet_only()
{
#ifdef HAVE_RMLUI
	if (!inventory_document) {
		return;
	}
	
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	Rml::Factory::ClearStyleSheetCache();
	inventory_document->ReloadStyleSheet();
#endif
}

time_t InventorySystem::get_file_mod_time(const std::string& filepath)
{
	struct stat file_stat;
	if (stat(filepath.c_str(), &file_stat) == 0) {
		return file_stat.st_mtime;
	}
	return 0;
}

#ifdef HAVE_RMLUI
void InventorySystem::ProcessEvent(Rml::Event& event)
{
	if (!inventory_document) return;
	
	Rml::Element* target = event.GetTargetElement();
	if (!target) return;
	
	Rml::String element_id = target->GetId();
	
	if (element_id == "weapons_tab") {
		if (Rml::Element* weapons_content = inventory_document->GetElementById("weapons_content")) {
			weapons_content->SetClass("visible", true);
			weapons_content->SetClass("hidden", false);
		}
		if (Rml::Element* upgrades_content = inventory_document->GetElementById("upgrades_content")) {
			upgrades_content->SetClass("visible", false);
			upgrades_content->SetClass("hidden", true);
		}

		if (Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab")) {
			weapons_tab->SetClass("active", true);
		}
		if (Rml::Element* upgrades_tab = inventory_document->GetElementById("upgrades_tab")) {
			upgrades_tab->SetClass("active", false);
		}
	}
	else if (element_id == "upgrades_tab") {
		if (Rml::Element* weapons_content = inventory_document->GetElementById("weapons_content")) {
			weapons_content->SetClass("visible", false);
			weapons_content->SetClass("hidden", true);
		}
		if (Rml::Element* upgrades_content = inventory_document->GetElementById("upgrades_content")) {
			upgrades_content->SetClass("visible", true);
			upgrades_content->SetClass("hidden", false);
		}

		if (Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab")) {
			weapons_tab->SetClass("active", false);
		}
		if (Rml::Element* upgrades_tab = inventory_document->GetElementById("upgrades_tab")) {
			upgrades_tab->SetClass("active", true);
		}

		update_ui_data();
	}
	else if (element_id == "close_inventory_btn") {
		hide_inventory();
		// Call the close callback if set (only when close button is pressed)
		if (on_close_callback) {
			on_close_callback(true);
		}
	}
	else if (element_id == "next_level_btn") {
		// Call the next level callback if set
		if (on_next_level_callback) {
			on_next_level_callback();
		}
		// Also close the inventory after triggering next level
		hide_inventory();
		if (on_close_callback) {
			on_close_callback(false);
		}
	}
	
	if (target->HasAttribute("data-weapon-id")) {
		unsigned int weapon_id = std::stoi(target->GetAttribute("data-weapon-id")->Get<Rml::String>());
		Entity player = registry.players.entities[0];
		
		Entity weapon_entity;
		bool found = false;
		for (Entity entity : registry.weapons.entities) {
			if ((unsigned int)entity == weapon_id) {
				weapon_entity = entity;
				found = true;
				break;
			}
		}
		
		if (!found) {
			return;
		}
		
		std::string classes = target->GetClassNames();
		
		if (classes.find("btn_buy") != std::string::npos) {
			buy_item(player, weapon_entity);
		} else if (classes.find("btn_equip") != std::string::npos) {
			equip_weapon(player, weapon_entity);
		}
	}
	else if (target->HasAttribute("data-weapon-upgrade")) {
		std::string upgrade_data = target->GetAttribute("data-weapon-upgrade")->Get<Rml::String>();
		// Format: "weapon_id:upgrade_type"
		size_t colon_pos = upgrade_data.find(':');
		if (colon_pos != std::string::npos) {
			unsigned int weapon_id = std::stoi(upgrade_data.substr(0, colon_pos));
			std::string upgrade_type = upgrade_data.substr(colon_pos + 1);
			
			Entity player = registry.players.entities[0];
			
			Entity weapon_entity;
			bool found = false;
			for (Entity entity : registry.weapons.entities) {
				if ((unsigned int)entity == weapon_id) {
					weapon_entity = entity;
					found = true;
					break;
				}
			}
			
			std::string classes = target->GetClassNames();
			if (found && classes.find("btn_upgrade") != std::string::npos) {
				buy_weapon_upgrade(player, weapon_entity, upgrade_type);
			}
		}
	}
	else if (target->HasAttribute("data-armour-id")) {
		unsigned int armour_id = std::stoi(target->GetAttribute("data-armour-id")->Get<Rml::String>());
		Entity player = registry.players.entities[0];

		Entity armour_entity;
		bool found = false;
		for (Entity entity : registry.armours.entities) {
			if ((unsigned int)entity == armour_id) {
				armour_entity = entity;
				found = true;
				break;
			}
		}

		if (!found) return;

		if (target->GetClassNames().find("btn_buy") != std::string::npos) {
			buy_item(player, armour_entity);
		} else if (target->GetClassNames().find("btn_equip") != std::string::npos) {
			equip_armour(player, armour_entity);
		}
	}
	else if (target->HasAttribute("data-upgrade-type")) {
		std::string upgrade_type = target->GetAttribute("data-upgrade-type")->Get<Rml::String>();
		Entity player = registry.players.entities[0];

		if (target->GetClassNames().find("btn_upgrade") != std::string::npos) {
			buy_upgrade(player, upgrade_type);
		}
	}
}
#endif

void InventorySystem::update_ui_data()
{
#ifdef HAVE_RMLUI
	if (!inventory_document || !rml_context) {
		std::cerr << "ERROR: Inventory document or context is null!" << std::endl;
		return;
	}

	if (registry.players.entities.empty()) {
		return;
	}

	Entity player_entity = registry.players.entities[0];
	if (!registry.inventories.has(player_entity)) {
		return;
	}
	
	Player& player = registry.players.get(player_entity);
	Inventory& inventory = registry.inventories.get(player_entity);

	if (Rml::Element* currency_text = inventory_document->GetElementById("currency_text")) {
		currency_text->SetInnerRML(std::to_string(player.currency));
	}

	Rml::Element* weapon_list = inventory_document->GetElementById("weapon_list");
	if (weapon_list) {
		std::string all_weapons_html = "";
		
		int weapon_index = 1;
		for (Entity weapon_entity : inventory.weapons) {
			if (!registry.weapons.has(weapon_entity)) {
				continue;
			}
			
			Weapon& weapon = registry.weapons.get(weapon_entity);
			
			// Get weapon upgrades (initialize if not present)
			if (!registry.weaponUpgrades.has(weapon_entity)) {
				registry.weaponUpgrades.emplace(weapon_entity);
			}
			WeaponUpgrades& upgrades = registry.weaponUpgrades.get(weapon_entity);
			
			// Calculate actual stats with upgrades
			int actual_damage = weapon.damage + (upgrades.damage_level * WeaponUpgrades::DAMAGE_PER_LEVEL);
			
			std::string button_markup;
			if (!weapon.owned) {
				if (weapon.price > 0) {
					button_markup = "<button class='btn btn_buy' data-weapon-id='" + 
					               std::to_string(weapon_entity) + "'>BUY " + std::to_string(weapon.price) + "</button>";
				} else {
					button_markup = "<button class='btn btn_locked' disabled='disabled'>LOCKED</button>";
				}
			} else if (weapon.equipped) {
				button_markup = "<button class='btn btn_equipped' disabled='disabled'>EQUIPPED</button>";
			} else {
				button_markup = "<button class='btn btn_equip' data-weapon-id='" + 
				               std::to_string(weapon_entity) + "'>EQUIP</button>";
			}
			
			// Determine weapon image based on weapon type
			std::string weapon_image_path = "";
			if (weapon.type == WeaponType::LASER_PISTOL_GREEN || weapon.type == WeaponType::LASER_PISTOL_RED) {
				weapon_image_path = "../data/textures/Weapons/laser_pistol.png";
			} else if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
				weapon_image_path = "../data/textures/Weapons/plasma_shotgun.png";
			} else if (weapon.type == WeaponType::ASSAULT_RIFLE || weapon.type == WeaponType::EXPLOSIVE_RIFLE) {
				weapon_image_path = "../data/textures/Weapons/assault_rifle.png";
			}
			
			std::string icon_html = "";
			if (!weapon_image_path.empty()) {
				icon_html = "<img src=\"" + weapon_image_path + "\" width=\"120\" height=\"100\" style=\"object-fit: contain; display: block;\" alt=\"\"/>";
			} else {
				icon_html = "<div class='weapon_icon_" + std::to_string(weapon_index) + "'></div>";
			}
			
			std::string damage_level_bar = "<div class='level_bar'>";
			for (int i = 0; i < WeaponUpgrades::MAX_UPGRADE_LEVEL; i++) {
				if (i <= upgrades.damage_level) {
					damage_level_bar += "<div class='level_tick filled'></div>";
				} else {
					damage_level_bar += "<div class='level_tick'></div>";
				}
			}
			damage_level_bar += "</div>";
			
			std::string magazine_level_bar = "<div class='level_bar'>";
			for (int i = 0; i < WeaponUpgrades::MAX_UPGRADE_LEVEL; i++) {
				if (i <= upgrades.ammo_capacity_level) {
					magazine_level_bar += "<div class='level_tick filled'></div>";
				} else {
					magazine_level_bar += "<div class='level_tick'></div>";
				}
			}
			magazine_level_bar += "</div>";
			
			std::string reload_level_bar = "<div class='level_bar'>";
			for (int i = 0; i < WeaponUpgrades::MAX_UPGRADE_LEVEL; i++) {
				if (i <= upgrades.reload_time_level) {
					reload_level_bar += "<div class='level_tick filled'></div>";
				} else {
					reload_level_bar += "<div class='level_tick'></div>";
				}
			}
			reload_level_bar += "</div>";
			
			std::string upgrade_buttons = "";
			if (weapon.owned) {
				std::string damage_button = "";
				if (upgrades.damage_level >= WeaponUpgrades::MAX_UPGRADE_LEVEL) {
					damage_button = "<button class='btn_upgrade' disabled style='align-self: flex-start; color: #ffd700;'>MAXED</button>";
				} else {
					damage_button = "<button class='btn_upgrade' data-weapon-upgrade='" + 
					               std::to_string(weapon_entity) + ":weapon_damage' style='align-self: flex-start;'>" + 
					               std::to_string(WeaponUpgrades::DAMAGE_COST) + "</button>";
				}
				
				std::string magazine_button = "";
				if (upgrades.ammo_capacity_level >= WeaponUpgrades::MAX_UPGRADE_LEVEL) {
					magazine_button = "<button class='btn_upgrade' disabled style='align-self: flex-start; color: #ffd700;'>MAXED</button>";
				} else {
					magazine_button = "<button class='btn_upgrade' data-weapon-upgrade='" + 
					                 std::to_string(weapon_entity) + ":weapon_magazine_size' style='align-self: flex-start;'>" + 
					                 std::to_string(WeaponUpgrades::AMMO_CAPACITY_COST) + "</button>";
				}
				
				std::string reload_button = "";
				if (upgrades.reload_time_level >= WeaponUpgrades::MAX_UPGRADE_LEVEL) {
					reload_button = "<button class='btn_upgrade' disabled style='align-self: flex-start; color: #ffd700;'>MAXED</button>";
				} else {
					reload_button = "<button class='btn_upgrade' data-weapon-upgrade='" + 
					               std::to_string(weapon_entity) + ":weapon_reload_time' style='align-self: flex-start;'>" + 
					               std::to_string(WeaponUpgrades::RELOAD_TIME_COST) + "</button>";
				}
				
				upgrade_buttons = 
					"<div class='weapon_upgrades' style='display: flex; gap: 50px; padding-top: 10px; padding-bottom: 10px;'>"
					"  <div class='weapon_stat_upgrade' style='display: flex; flex-direction: column; gap: 5px;'>"
					"    <div class='weapon_stat_label'>Damage</div>"
					+ damage_level_bar +
					"    " + damage_button +
					"  </div>"
					"  <div class='weapon_stat_upgrade' style='display: flex; flex-direction: column; gap: 5px;'>"
					"    <div class='weapon_stat_label'>Magazine Size</div>"
					+ magazine_level_bar +
					"    " + magazine_button +
					"  </div>"
					"  <div class='weapon_stat_upgrade' style='display: flex; flex-direction: column; gap: 5px;'>"
					"    <div class='weapon_stat_label'>Reload Time</div>"
					+ reload_level_bar +
					"    " + reload_button +
					"  </div>"
					"</div>";
			}
			
			std::string item_html = 
				"<div class='item_row'>"
				"  <div class='item_icon'>" + icon_html + "</div>"
				"  <div class='item_info'>"
				"    <div class='item_name'>" + weapon.name + "</div>"
				"    <div class='item_description'>" + weapon.description + "</div>"
				+ upgrade_buttons +
				"  </div>"
				+ button_markup +
				"</div>";
			
			all_weapons_html += item_html;
			weapon_index++;
		}
		
		weapon_list->SetInnerRML(all_weapons_html);
	} else {
		std::cerr << "ERROR: weapon_list element not found in document!" << std::endl;
	}


	// Build the upgrade cards UI
	if (!registry.playerUpgrades.has(player_entity)) {
		registry.playerUpgrades.emplace(player_entity);
	}

	PlayerUpgrades& player_upgrades = registry.playerUpgrades.get(player_entity);

	Rml::Element* player_upgrade_list = inventory_document->GetElementById("player_upgrade_list");
	if (player_upgrade_list) {
		std::string all_upgrades_html = "";

		// Struct to hold all the info we need to display each upgrade card
		struct UpgradeInfo {
			std::string name;
			std::string description;
			int current_level;
			int cost;
			std::string upgrade_type;
			std::string icon_path;
		};

		// Array of all upgrades with their display info
		UpgradeInfo upgrades[] = {
			{
				"Max Health",
				"Increases maximum health by " + std::to_string(PlayerUpgrades::HEALTH_PER_LEVEL) + " per level",
				player_upgrades.max_health_level,
				PlayerUpgrades::MAX_HEALTH_COST,
				"max_health",
				"../data/textures/Upgrades/max_health.png"
			},
			{
				"Armour",
				"Adds flat damage reduction",
				player_upgrades.armour_level,
				PlayerUpgrades::ARMOUR_COST,
				"armour",
				"../data/textures/Upgrades/armour.png"
			},
			{
				"Health Regen",
				"Regenerate " + std::to_string((int)PlayerUpgrades::HEALTH_REGEN_PER_LEVEL) + " HP per second",
				player_upgrades.health_regen_level,
				PlayerUpgrades::HEALTH_REGEN_COST,
				"health_regen",
				"../data/textures/Upgrades/health_regen.png"
			},
			{
				"Life Steal",
				"Heal for " + std::to_string((int)(PlayerUpgrades::LIFE_STEAL_PER_LEVEL * 100)) + "% of damage dealt per level",
				player_upgrades.life_steal_level,
				PlayerUpgrades::LIFE_STEAL_COST,
				"life_steal",
				"../data/textures/Upgrades/lifesteal.png"
			},
			{
				"Movement Speed",
				"Increases movement speed by " + std::to_string((int)PlayerUpgrades::MOVEMENT_SPEED_PER_LEVEL) + " per level",
				player_upgrades.movement_speed_level,
				PlayerUpgrades::MOVEMENT_SPEED_COST,
				"movement_speed",
				"../data/textures/Upgrades/movement_speed.png"
			},
			{
				"Dash Cooldown",
				"Reduces dash recovery time by " + std::to_string((int)(PlayerUpgrades::DASH_COOLDOWN_REDUCTION_PER_LEVEL * 100)) + "% per level",
				player_upgrades.dash_cooldown_level,
				PlayerUpgrades::DASH_COOLDOWN_COST,
				"dash_cooldown",
				"../data/textures/Upgrades/dash_cooldown.png"
			},
			{
				"Light Radius",
				"Extends flashlight range by " + std::to_string((int)PlayerUpgrades::LIGHT_RADIUS_PER_LEVEL) + " per level",
				player_upgrades.light_radius_level,
				PlayerUpgrades::LIGHT_RADIUS_COST,
				"light_radius",
				"../data/textures/Upgrades/flashlight_radius.png"
			},
			{
				"Flashlight Width",
				"Widens flashlight beam to slow more enemies",
				player_upgrades.flashlight_width_level,
				PlayerUpgrades::FLASHLIGHT_WIDTH_COST,
				"flashlight_width",
				"../data/textures/Upgrades/flashlight_width.png"
			},
			{
				"Critical Hit",
				"+" + std::to_string((int)(PlayerUpgrades::CRIT_CHANCE_PER_LEVEL * 100)) + "% chance to deal double damage per level",
				player_upgrades.crit_chance_level,
				PlayerUpgrades::CRIT_CHANCE_COST,
				"crit_chance",
				"../data/textures/Upgrades/crit.png"
			},
			{
				"Flashlight Burn",
				"Damages enemies in flashlight beam over time",
				player_upgrades.flashlight_damage_level,
				PlayerUpgrades::FLASHLIGHT_DAMAGE_COST,
				"flashlight_damage",
				"../data/textures/Upgrades/flashlight_burn.png"
			},
			{
				"Flashlight Freeze",
				"Slows enemies in beam even more",
				player_upgrades.flashlight_slow_level,
				PlayerUpgrades::FLASHLIGHT_SLOW_COST,
				"flashlight_slow",
				"../data/textures/Upgrades/flashlight_freeze.png"
			},
			{
				"Xylarite Gain",
				"+" + std::to_string((int)(PlayerUpgrades::XYLARITE_MULTIPLIER_PER_LEVEL * 100)) + "% xylarite per pickup",
				player_upgrades.xylarite_multiplier_level,
				PlayerUpgrades::XYLARITE_MULTIPLIER_COST,
				"xylarite_multiplier",
				"../data/textures/Upgrades/xylarite_multiplier.png"
			}
		};

		// Generate HTML for each upgrade card
		for (const auto& upgrade : upgrades) {

			// Build the level bar showing filled and empty ticks
			std::string level_bar_html = "<div class='level_bar'>";
			for (int i = 0; i < PlayerUpgrades::MAX_UPGRADE_LEVEL; i++) {
				if (i < upgrade.current_level) {
					level_bar_html += "<div class='level_tick filled'></div>";
				} else {
					level_bar_html += "<div class='level_tick'></div>";
				}
			}
			level_bar_html += "</div>";

			// Show maxed text or buy button depending on level
			std::string button_or_maxed;
			if (upgrade.current_level >= PlayerUpgrades::MAX_UPGRADE_LEVEL) {
				button_or_maxed = "<div class='upgrade_maxed'>MAXED</div>";
			} else {
				button_or_maxed = "<button class='btn_upgrade' data-upgrade-type='" + upgrade.upgrade_type + "'>" + std::to_string(upgrade.cost) + "</button>";
			}

			// Show icon image or black box if no icon
			std::string icon_html;
			if (upgrade.icon_path.empty()) {
				icon_html = "<div class='upgrade_icon' style='background-color: #000000;'></div>";
			} else {
				icon_html = "<img class='upgrade_icon' src='" + upgrade.icon_path + "'/>";
			}

			// Assemble the full card HTML
			std::string card_html =
				"<div class='upgrade_card'>"
				"  <div class='upgrade_tooltip'>" + upgrade.description + "</div>"
				+ icon_html +
				"  <div class='upgrade_name'>" + upgrade.name + "</div>"
				+ level_bar_html +
				button_or_maxed +
				"</div>";

			all_upgrades_html += card_html;
		}

		player_upgrade_list->SetInnerRML(all_upgrades_html);
	}
#endif
}

#ifdef HAVE_RMLUI
double RmlSystemInterface::GetElapsedTime()
{
	return glfwGetTime();
}

bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& message)
{
	if (type == Rml::Log::LT_ERROR) {
		std::cerr << "RmlUi Error: " << message << std::endl;
		return true;
	}
	return true;
}

bool InventorySystem::is_mouse_over_button(vec2 mouse_position)
{
#ifdef HAVE_RMLUI
	if (!inventory_document || !inventory_open) {
		return false;
	}
	
	auto check_hover = [](Rml::Element* element) -> bool {
		if (!element) return false;
		return element->IsPseudoClassSet("hover");
	};
	
	Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab");
	Rml::Element* upgrades_tab = inventory_document->GetElementById("upgrades_tab");
	Rml::Element* suits_tab = inventory_document->GetElementById("suits_tab");
	Rml::Element* close_btn = inventory_document->GetElementById("close_inventory_btn");
	Rml::Element* next_level_btn = inventory_document->GetElementById("next_level_btn");

	if (check_hover(weapons_tab) || check_hover(upgrades_tab) || check_hover(suits_tab) || check_hover(close_btn) || check_hover(next_level_btn)) {
		return true;
	}
	
	Rml::ElementList buttons;
	inventory_document->GetElementsByTagName(buttons, "button");
	for (Rml::Element* button : buttons) {
		if (check_hover(button)) {
			return true;
		}
	}
#endif
	
	return false;
}

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
	
	std::string vs_path = shader_path("ui") + ".vs.glsl";
	std::string fs_path = shader_path("ui") + ".fs.glsl";
	
	if (!loadEffectFromFile(vs_path, fs_path, shader_program)) {
		std::cerr << "Failed to load UI shader" << std::endl;
		return false;
	}
	
	projection_uniform = glGetUniformLocation(shader_program, "projection");
	translation_uniform = glGetUniformLocation(shader_program, "translation");
	use_texture_uniform = glGetUniformLocation(shader_program, "use_texture");
	GLint tex_uniform = glGetUniformLocation(shader_program, "tex");
	
	glUseProgram(shader_program);
	if (tex_uniform != -1) {
		glUniform1i(tex_uniform, 0);
	}
	glUseProgram(0);
	
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
	
	glUseProgram(shader_program);
	
	if (projection_uniform == -1 || translation_uniform == -1 || use_texture_uniform == -1) {
		std::cerr << "Invalid uniform locations: proj=" << projection_uniform 
		          << " trans=" << translation_uniform 
		          << " tex=" << use_texture_uniform << std::endl;
		return;
	}
	
	float projection_matrix[16] = {
		2.0f / width, 0.0f,            0.0f, 0.0f,
		0.0f,         -2.0f / height,  0.0f, 0.0f,
		0.0f,         0.0f,            1.0f, 0.0f,
		-1.0f,        1.0f,            0.0f, 1.0f
	};
	
	glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, projection_matrix);
	glUniform2f(translation_uniform, translation.x, translation.y);
	
	bool has_texture = (texture != 0);
	glUniform1i(use_texture_uniform, has_texture ? 1 : 0);
	
	glActiveTexture(GL_TEXTURE0);
	if (has_texture) {
		glBindTexture(GL_TEXTURE_2D, (GLuint)texture);
	} else {
		glBindTexture(GL_TEXTURE_2D, dummy_texture);
	}
	
	glBindVertexArray(geom->vao);
	glDrawElements(GL_TRIANGLES, geom->num_indices, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	
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
	int gl_y = height - (region.Top() + region.Height());
	glScissor(region.Left(), gl_y, region.Width(), region.Height());
}

Rml::TextureHandle RmlRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions,
                                                     const Rml::String& source)
{
	// Use stb_image to load the texture (already included at top of file)
	int width, height, channels;
	unsigned char* image_data = stbi_load(source.c_str(), &width, &height, &channels, 4); // Force RGBA
	
	if (!image_data) {
		std::cerr << "ERROR: Failed to load RmlUi texture: " << source << std::endl;
		std::cerr << "STB Error: " << stbi_failure_reason() << std::endl;
		return 0;
	}
	
	texture_dimensions = Rml::Vector2i(width, height);
	
	for (int i = 0; i < width * height * 4; i += 4) {
		unsigned char alpha = image_data[i + 3];
		image_data[i + 0] = (image_data[i + 0] * alpha) / 255;
		image_data[i + 1] = (image_data[i + 1] * alpha) / 255;
		image_data[i + 2] = (image_data[i + 2] * alpha) / 255;
	}
	
	// Create OpenGL texture
	GLuint texture_id = 0;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	glBindTexture(GL_TEXTURE_2D, 0);
	
	stbi_image_free(image_data);
	
	return (Rml::TextureHandle)texture_id;
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

