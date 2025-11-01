#include "inventory_system.hpp"
#include "tiny_ecs_registry.hpp"
#include "render_system.hpp"
#include <iostream>
#include <sys/stat.h>
#include <thread>
#include <chrono>

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
	if (inventory_document) {
		inventory_document->Close();
	}
	Rml::Shutdown();
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

	int width = window_width_px;
	int height = window_height_px; 

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
	create_default_armors();
	
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
		{WeaponType::LASER_PISTOL_GREEN, "Laser Pistol", "Base Pistol, reliable accurate.", 10, 0, true},
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

	Inventory& inventory = registry.inventories.emplace(player_entity);
	
	for (Entity weapon_entity : registry.weapons.entities) {
		inventory.weapons.push_back(weapon_entity);
		
		Weapon& weapon = registry.weapons.get(weapon_entity);
		if (weapon.equipped) {
			inventory.equipped_weapon = weapon_entity;
		}
	}

	for (Entity armor_entity : registry.armors.entities) {
		inventory.armors.push_back(armor_entity);
		
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
		if (Rml::Element* suits_tab = inventory_document->GetElementById("suits_tab")) {
			suits_tab->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* close_btn = inventory_document->GetElementById("close_btn")) {
			close_btn->AddEventListener(Rml::EventId::Click, this);
		}
		
		if (Rml::Element* weapon_list = inventory_document->GetElementById("weapon_list")) {
			weapon_list->AddEventListener(Rml::EventId::Click, this);
		}
		if (Rml::Element* suit_list = inventory_document->GetElementById("suit_list")) {
			suit_list->AddEventListener(Rml::EventId::Click, this);
		}
	}
	
	inventory_open = true;
	
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
	
	if (window && is_hovering_button) {
		glfwSetCursor(window, nullptr);
		is_hovering_button = false;
	}
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
	
	if (!armor.owned) {
		return;
	}

	Inventory& inventory = registry.inventories.get(player_entity);

	if (registry.armors.has(inventory.equipped_armor)) {
		registry.armors.get(inventory.equipped_armor).equipped = false;
	}

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

	if (registry.weapons.has(item_entity)) {
		Weapon& weapon = registry.weapons.get(item_entity);
		
		if (weapon.owned) {
			return false;
		}

		if (player.currency >= weapon.price) {
			player.currency -= weapon.price;
			weapon.owned = true;
			update_ui_data();
			return true;
		}
		return false;
	}

	if (registry.armors.has(item_entity)) {
		Armor& armor = registry.armors.get(item_entity);
		
		if (armor.owned) {
			return false;
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
	if (Rml::Element* suits_tab = inventory_document->GetElementById("suits_tab")) {
		suits_tab->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* close_btn = inventory_document->GetElementById("close_btn")) {
		close_btn->AddEventListener(Rml::EventId::Click, this);
	}
	
	if (Rml::Element* weapon_list = inventory_document->GetElementById("weapon_list")) {
		weapon_list->AddEventListener(Rml::EventId::Click, this);
	}
	if (Rml::Element* suit_list = inventory_document->GetElementById("suit_list")) {
		suit_list->AddEventListener(Rml::EventId::Click, this);
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
			weapons_content->SetProperty("display", "block");
		}
		if (Rml::Element* suits_content = inventory_document->GetElementById("suits_content")) {
			suits_content->SetProperty("display", "none");
		}
		
		if (Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab")) {
			weapons_tab->SetClass("active", true);
		}
		if (Rml::Element* suits_tab = inventory_document->GetElementById("suits_tab")) {
			suits_tab->SetClass("active", false);
		}
	}
	else if (element_id == "suits_tab") {
		if (Rml::Element* weapons_content = inventory_document->GetElementById("weapons_content")) {
			weapons_content->SetProperty("display", "none");
		}
		if (Rml::Element* suits_content = inventory_document->GetElementById("suits_content")) {
			suits_content->SetProperty("display", "block");
		}
		
		if (Rml::Element* weapons_tab = inventory_document->GetElementById("weapons_tab")) {
			weapons_tab->SetClass("active", false);
		}
		if (Rml::Element* suits_tab = inventory_document->GetElementById("suits_tab")) {
			suits_tab->SetClass("active", true);
		}
	}
	else if (element_id == "close_btn") {
		hide_inventory();
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
	else if (target->HasAttribute("data-armor-id")) {
		unsigned int armor_id = std::stoi(target->GetAttribute("data-armor-id")->Get<Rml::String>());
		Entity player = registry.players.entities[0];
		
		Entity armor_entity;
		bool found = false;
		for (Entity entity : registry.armors.entities) {
			if ((unsigned int)entity == armor_id) {
				armor_entity = entity;
				found = true;
				break;
			}
		}
		
		if (!found) return;
		
		if (target->GetClassNames().find("btn_buy") != std::string::npos) {
			buy_item(player, armor_entity);
		} else if (target->GetClassNames().find("btn_equip") != std::string::npos) {
			equip_armor(player, armor_entity);
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
			
			std::string item_html = 
				"<div class='item_row'>"
				"  <div class='item_icon weapon_icon_" + std::to_string(weapon_index) + "'></div>"
				"  <div class='item_info'>"
				"    <div class='item_name'>" + weapon.name + "</div>"
				"    <div class='item_description'>" + weapon.description + "</div>"
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

	Rml::Element* suit_list = inventory_document->GetElementById("suit_list");
	if (suit_list) {
		std::string all_suits_html = "";
		
		int armor_index = 1;
		for (Entity armor_entity : inventory.armors) {
			if (!registry.armors.has(armor_entity)) continue;
			
			Armor& armor = registry.armors.get(armor_entity);
			
			std::string button_markup;
			if (!armor.owned) {
				if (armor.price > 0) {
					button_markup = "<button class='btn btn_buy' data-armor-id='" + 
					               std::to_string(armor_entity) + "'>BUY " + 
					               std::to_string(armor.price) + "</button>";
				} else {
					button_markup = "<button class='btn btn_locked' disabled='disabled'>LOCKED</button>";
				}
			} else if (armor.equipped) {
				button_markup = "<button class='btn btn_equipped' disabled='disabled'>EQUIPPED</button>";
			} else {
				button_markup = "<button class='btn btn_equip' data-armor-id='" + 
				               std::to_string(armor_entity) + "'>EQUIP</button>";
			}
			
			std::string item_html = 
				"<div class='item_row'>"
				"  <div class='item_icon suit_icon_" + std::to_string(armor_index) + "'></div>"
				"  <div class='item_info'>"
				"    <div class='item_name'>" + armor.name + "</div>"
				"    <div class='item_description'>" + armor.description + "</div>"
				"  </div>"
				+ button_markup +
				"</div>";
			
			all_suits_html += item_html;
			armor_index++;
		}
		
		suit_list->SetInnerRML(all_suits_html);
	} else {
		std::cerr << "ERROR: suit_list element not found in document!" << std::endl;
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
	Rml::Element* suits_tab = inventory_document->GetElementById("suits_tab");
	Rml::Element* close_btn = inventory_document->GetElementById("close_btn");
	
	if (check_hover(weapons_tab) || check_hover(suits_tab) || check_hover(close_btn)) {
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

