// Header
#include "world_system.hpp"
#include "world_init.hpp"
#include "noise_gen.hpp"

// stlib
#include <cassert>
#include <sstream>
#include <cmath>
#include <iostream>
#include <thread>
#include <chrono>

#include "physics_system.hpp"
#include "ai_system.hpp"
#include "start_menu_system.hpp"

#ifdef HAVE_RMLUI
#include <RmlUi/Core.h>
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923  // pi/2
#endif
#ifndef M_PI_4
#define M_PI_4 0.78539816339744830961  // pi/4
#endif

// create the underwater world
WorldSystem::WorldSystem() :
	points(0),
	left_pressed(false),
	right_pressed(false),
	up_pressed(false),
	down_pressed(false),
	prioritize_right(false),
	prioritize_down(false),
	mouse_pos(vec2(0,0)),
	is_dashing(false),
	dash_timer(0.0f),
	dash_cooldown_timer(0.0f),
	is_knockback(false),
	knockback_timer(0.0f),
	knockback_direction(vec2(0,0)),
	is_hurt_knockback(false),
	hurt_knockback_timer(0.0f),
	hurt_knockback_direction(vec2(0,0)),
	animation_before_hurt(TEXTURE_ASSET_ID::PLAYER_IDLE)
{
	// Seeding rng with random device
	rng = std::default_random_engine(std::random_device()());
}

WorldSystem::~WorldSystem() {
	

	// Destroy all created components
	registry.clear_all_components();

	// Close the window
	glfwDestroyWindow(window);
}

// Debugging
namespace {
	void glfw_err_cb(int error, const char *desc) {
		fprintf(stderr, "%d: %s", error, desc);
	}

	void sync_flashlight_to_player(const Motion& player_motion, Motion& flashlight_motion, vec2 additional_offset = vec2{0.f, 0.f})
	{
		float c = cos(player_motion.angle);
		float s = sin(player_motion.angle);

		float forward_dist = player_motion.scale.x * 0.45f;
		float lateral_offset = player_motion.scale.x * 0.1f;

		vec2 forward_vec = { c * forward_dist, s * forward_dist };
		vec2 lateral_vec = { -s * lateral_offset, c * lateral_offset };

		vec2 muzzle_offset = forward_vec + lateral_vec;
		vec2 muzzle_pos = player_motion.position + muzzle_offset;

		float tip_local_x = 6.0f;
		float tip_local_y = -1.0f;
		
		vec2 tip_offset_rotated = {
			tip_local_x * c - tip_local_y * s,
			tip_local_x * s + tip_local_y * c
		};
		

		tip_offset_rotated.x *= flashlight_motion.scale.x;
		tip_offset_rotated.y *= flashlight_motion.scale.y;

		flashlight_motion.position = muzzle_pos - tip_offset_rotated + additional_offset;

		flashlight_motion.angle = player_motion.angle;
		flashlight_motion.velocity = {0.f, 0.f};
	}

}

// World initialization
// Note, this has a lot of OpenGL specific things, could be moved to the renderer
GLFWwindow* WorldSystem::create_window() {
	///////////////////////////////////////
	// Initialize GLFW
	glfwSetErrorCallback(glfw_err_cb);
	if (!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW");
		return nullptr;
	}

	//-------------------------------------------------------------------------
	// If you are on Linux or Windows, you can change these 2 numbers to 4 and 3 and
	// enable the glDebugMessageCallback to have OpenGL catch your mistakes for you.
	// GLFW / OGL Initialization
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#if __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, 0);

	// Create the main window (for rendering, keyboard, and mouse input)
	window = glfwCreateWindow(window_width_px, window_height_px, "Eclipse", nullptr, nullptr);
	if (window == nullptr) {
		fprintf(stderr, "Failed to glfwCreateWindow");
		return nullptr;
	}

	// Setting callbacks to member functions (that's why the redirect is needed)
	// Input is handled using GLFW, for more info see
	// http://www.glfw.org/docs/latest/input_guide.html
	glfwSetWindowUserPointer(window, this);
	auto key_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2, int _3) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_key(_0, _1, _2, _3); };
	auto cursor_pos_redirect = [](GLFWwindow* wnd, double _0, double _1) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_move({ _0, _1 }); };
	auto mouse_button_redirect = [](GLFWwindow* wnd, int _0, int _1, int _2) { ((WorldSystem*)glfwGetWindowUserPointer(wnd))->on_mouse_click(_0, _1, _2); };
	glfwSetKeyCallback(window, key_redirect);
	glfwSetCursorPosCallback(window, cursor_pos_redirect);
	glfwSetMouseButtonCallback(window, mouse_button_redirect);


	return window;
}

void WorldSystem::init(RenderSystem* renderer_arg, InventorySystem* inventory_arg, StatsSystem* stats_arg, ObjectivesSystem* objectives_arg, MinimapSystem* minimap_arg, CurrencySystem* currency_arg, MenuIconsSystem* menu_icons_arg, TutorialSystem* tutorial_arg, StartMenuSystem* start_menu_arg, AISystem* ai_arg, AudioSystem* audio_arg) {
	this->renderer = renderer_arg;
	this->inventory_system = inventory_arg;
	this->stats_system = stats_arg;
	this->objectives_system = objectives_arg;
	this->minimap_system = minimap_arg;
	this->currency_system = currency_arg;
	this->menu_icons_system = menu_icons_arg;
	this->tutorial_system = tutorial_arg;
	this->start_menu_system = start_menu_arg;
	this->audio_system = audio_arg;

	if (start_menu_system && !start_menu_system->is_supported()) {
		start_menu_system = nullptr;
	}

	// Pass window handle to inventory system for cursor management
	if (inventory_system && window) {
		inventory_system->set_window(window);
		// Set callback to exit bonfire mode when inventory is closed
		inventory_system->set_on_close_callback([this]() {
			this->exit_bonfire_mode();
		});
	}
	
	// Set up kill callback for AI system
	if (ai_arg) {
		ai_arg->set_kill_callback([this]() {
			this->kill_count++;
		});
	}

	// Level display is now part of the shared HUD document managed by CurrencySystem
	// Initialize level display after currency system is set up
	if (currency_system) {
		update_level_display();
	}
	
	// Initialize level transition splash screen
#ifdef HAVE_RMLUI
	if (inventory_system) {
		Rml::Context* rml_context = inventory_system->get_context();
		if (rml_context) {
			level_transition_document = rml_context->LoadDocument("ui/level_transition.rml");
			if (!level_transition_document) {
				level_transition_document = rml_context->LoadDocument("../ui/level_transition.rml");
			}
			if (level_transition_document) {
				level_transition_document->Show();
				// Initially hidden via CSS class
				Rml::Element* container = level_transition_document->GetElementById("level_transition_container");
				if (container) {
					container->SetClass("visible", false);
				}
			}
		}
	}
#endif
	
	// Initialize bonfire instructions UI
#ifdef HAVE_RMLUI
	if (inventory_system) {
		Rml::Context* rml_context = inventory_system->get_context();
		if (rml_context) {
			bonfire_instructions_document = rml_context->LoadDocument("ui/bonfire_instructions.rml");
			if (!bonfire_instructions_document) {
				bonfire_instructions_document = rml_context->LoadDocument("../ui/bonfire_instructions.rml");
			}
			if (bonfire_instructions_document) {
				bonfire_instructions_document->Show();
				// Initially hidden via CSS class
				Rml::Element* container = bonfire_instructions_document->GetElementById("bonfire_instructions_container");
				if (container) {
					container->SetClass("visible", false);
				}
			}
		}
	}
#endif

	this->map_perlin = PerlinNoiseGenerator();
	this->decorator_perlin = PerlinNoiseGenerator();

	// Set all states to default
	gameplay_started = (start_menu_system == nullptr);
	start_menu_active = (start_menu_system != nullptr);
	start_menu_transitioning = false;
	hud_intro_played = false;

	restart_game();

	if (start_menu_system) {
		start_menu_system->set_start_game_callback([this]() {
			this->request_start_game();
		});
		start_menu_system->set_continue_callback([this]() {
			this->request_start_game();
		});
		start_menu_system->set_exit_callback([this]() {
			if (window) {
				glfwSetWindowShouldClose(window, true);
			}
		});
		start_menu_system->set_menu_hidden_callback([this]() {
			start_menu_active = false;
			start_menu_transitioning = false;
			start_camera_lerping = false;
			gameplay_started = true;
			if (renderer && registry.motions.has(player_salmon)) {
				renderer->setCameraPosition(registry.motions.get(player_salmon).position);
			}
			if (!hud_intro_played) {
				play_hud_intro();
				hud_intro_played = true;
			}
			if (tutorial_system) {
				tutorial_system->start_tutorial();
			}
		});
		start_menu_system->set_open_settings_callback([]() {
			std::cout << "[StartMenu] Settings menu not implemented yet.\n";
		});
		start_menu_system->set_open_tutorials_callback([this]() {
			if (tutorial_system) {
				tutorial_system->start_tutorial();
			}
		});
		
		// Hide all HUD elements when start menu is shown
		if (stats_system) {
			stats_system->set_visible(false);
		}
		if (minimap_system) {
			minimap_system->set_visible(false);
		}
		if (currency_system) {
			currency_system->set_visible(false);
		}
		if (objectives_system) {
			objectives_system->set_visible(false);
		}
		if (menu_icons_system) {
			menu_icons_system->set_visible(false);
		}
		if (inventory_system && inventory_system->is_inventory_open()) {
			inventory_system->toggle_inventory();
		}
		
		start_menu_system->show();
		
		// Set up menu icons callbacks
		if (menu_icons_system) {
			menu_icons_system->set_return_to_menu_callback([this]() {
				this->request_return_to_menu();
			});
		}
	} else {
		play_hud_intro();
		hud_intro_played = true;
		if (tutorial_system) {
			tutorial_system->start_tutorial();
		}
	}
}

void WorldSystem::request_start_game()
{
	if (!start_menu_active || start_menu_transitioning) {
		return;
	}

	start_menu_transitioning = true;

	if (registry.motions.has(player_salmon)) {
		Motion& player_motion = registry.motions.get(player_salmon);
		start_camera_lerping = true;
		start_camera_lerp_time = 0.f;
		start_camera_lerp_start = start_menu_camera_focus;
		start_camera_lerp_target = player_motion.position;
	}

	if (start_menu_system) {
		start_menu_system->begin_exit_sequence();
	}

	if (!hud_intro_played) {
		play_hud_intro();
		hud_intro_played = true;
	}
}

void WorldSystem::request_return_to_menu()
{
	if (start_menu_active) {
		return;
	}

	if (tutorial_system && tutorial_system->is_active()) {
		tutorial_system->skip_tutorial();
	}

	hide_bonfire_instructions();

#ifdef HAVE_RMLUI
	if (level_transition_document && is_level_transitioning) {
		Rml::Element* container = level_transition_document->GetElementById("level_transition_container");
		if (container) {
			container->SetClass("visible", false);
		}
		is_level_transitioning = false;
		level_transition_timer = 0.0f;
	}
#endif

	if (stats_system) {
		stats_system->set_visible(false);
	}
	if (minimap_system) {
		minimap_system->set_visible(false);
	}
	if (currency_system) {
		currency_system->set_visible(false);
	}
	if (objectives_system) {
		objectives_system->set_visible(false);
	}
	if (menu_icons_system) {
		menu_icons_system->set_visible(false);
	}
	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->toggle_inventory();
	}

	if (start_menu_system) {
		start_menu_system->show();
	}

	start_menu_active = true;
	gameplay_started = false;
	start_menu_transitioning = false;
	start_camera_lerping = false;

	if (registry.motions.has(player_salmon)) {
		Motion& player_motion = registry.motions.get(player_salmon);
		vec2 start_screen_offset = { window_width_px * 0.28f, window_height_px * 0.12f };
		start_menu_camera_focus = {
			player_motion.position.x - start_screen_offset.x,
			player_motion.position.y - start_screen_offset.y
		};
	}

	if (renderer) {
		renderer->setCameraPosition(start_menu_camera_focus);
	}
}

void WorldSystem::finalize_start_menu_transition()
{
	if (!start_menu_active && !start_menu_transitioning) {
		return;
	}

	start_menu_active = false;
	start_menu_transitioning = false;
	start_camera_lerping = false;

	if (start_menu_system) {
		start_menu_system->hide_immediately();
	}

	if (window) {
		double cursor_x = mouse_pos.x;
		double cursor_y = mouse_pos.y;
		glfwGetCursorPos(window, &cursor_x, &cursor_y);
		mouse_pos.x = static_cast<float>(cursor_x);
		mouse_pos.y = static_cast<float>(cursor_y);
	}

	play_hud_intro();
	if (!hud_intro_played) {
		hud_intro_played = true;
	}

	if (tutorial_system && !tutorial_system->is_active()) {
		tutorial_system->start_tutorial();
	}
}

// Update our game world
bool WorldSystem::step(float elapsed_ms_since_last_update) {
	// update current time in seconds
	current_time_seconds += elapsed_ms_since_last_update / 1000.0f;
	
	// Updating window title with points
	std::stringstream title_ss;
	title_ss << "Points: " << points;
	glfwSetWindowTitle(window, title_ss.str().c_str());

	// Remove debug info from the last step
	while (registry.debugComponents.entities.size() > 0)
	    registry.remove_all_components_of(registry.debugComponents.entities.back());

	// Removing out of screen entities
	auto& motions_registry = registry.motions;

	// Ensure arrow is static and not affected by player controls
	// CRITICAL: Arrow must NEVER be treated as a player or get player controls
	if (arrow_exists && registry.motions.has(arrow_entity) && registry.arrows.has(arrow_entity)) {
		// Safety check: arrow should never be a player entity
		if (registry.players.has(arrow_entity)) {
			std::cerr << "ERROR: Arrow entity has player component! Removing it." << std::endl;
			registry.players.remove(arrow_entity);
		}
		
		Motion& arrow_motion = registry.motions.get(arrow_entity);
		
		// Debug: Check if velocity is non-zero (should never happen)
		if (arrow_motion.velocity.x != 0.f || arrow_motion.velocity.y != 0.f) {
			std::cerr << "WARNING: Arrow velocity is non-zero! (" << arrow_motion.velocity.x 
			          << ", " << arrow_motion.velocity.y << ") - Resetting to zero." << std::endl;
		}
		
		// Force velocity to zero - arrow is completely static
		arrow_motion.velocity = { 0.f, 0.f };
		// Arrow position will be set at the end of this function to camera position
	}

	// Handle player motion - ONLY for player_salmon entity
	auto& motion = motions_registry.get(player_salmon);
	
	// Safety check: ensure player_salmon is not the arrow
	if (arrow_exists && player_salmon == arrow_entity) {
		std::cerr << "ERROR: player_salmon is the same as arrow_entity!" << std::endl;
	}
	auto& sprite = registry.sprites.get(player_salmon);
	auto& render_request = registry.renderRequests.get(player_salmon);

	if (is_camera_lerping_to_bonfire) {
		camera_lerp_time += elapsed_ms_since_last_update;
		float t = camera_lerp_time / CAMERA_LERP_DURATION;
		if (t >= 1.0f) {
			t = 1.0f;
			is_camera_lerping_to_bonfire = false;
			vec2 current_player_pos = motion.position;
			vec2 diff = camera_lerp_target - current_player_pos;
			float dist_to_player = sqrt(diff.x * diff.x + diff.y * diff.y);
			if (dist_to_player > 50.0f) {
				is_camera_locked_on_bonfire = true;
			} else {
				is_camera_locked_on_bonfire = false;
			}
			
			// Open inventory after both lerping animations complete (camera lerp finishes last)
			if (should_open_inventory_after_lerp && !is_player_angle_lerping) {
				if (inventory_system && !inventory_system->is_inventory_open()) {
					// Note: Objectives are already reset when bonfire is interacted with
					// Change bonfire texture to "off" state when inventory opens
					if (bonfire_exists && registry.renderRequests.has(bonfire_entity)) {
						RenderRequest& bonfire_req = registry.renderRequests.get(bonfire_entity);
						bonfire_req.used_texture = TEXTURE_ASSET_ID::BONFIRE_OFF;
					}
					// Hide bonfire marker from minimap when it's disabled
					if (minimap_system) {
						float current_spawn_radius = level_manager.get_spawn_radius();
						vec2 current_spawn_position = { window_width_px/2.0f, window_height_px - 200.0f };
						minimap_system->update_bonfire_position(vec2(0.0f, 0.0f), current_spawn_radius, current_spawn_position);
					}
					// Open the inventory
					inventory_system->show_inventory();
				}
				should_open_inventory_after_lerp = false;
			}
		}
		t = t * t * (3.0f - 2.0f * t);
		vec2 current_camera_pos = camera_lerp_start + (camera_lerp_target - camera_lerp_start) * t;
		renderer->setCameraPosition(current_camera_pos);
	} else if (is_camera_locked_on_bonfire) {
		renderer->setCameraPosition(camera_lerp_target);
	} else {
		renderer->setCameraPosition(motion.position);
	}

	// feet motion and animation
	auto& feet_motion = motions_registry.get(player_feet);
	auto& feet_sprite = registry.sprites.get(player_feet);
	auto& feet_render_request = registry.renderRequests.get(player_feet);

	// dash motion and render
	auto& dash_motion = motions_registry.get(player_dash);
	auto& dash_render_request = registry.renderRequests.get(player_dash);

	// flashlight motion
	auto& flashlight_motion = motions_registry.get(flashlight);
	
	float salmon_vel = 200.0f;

	// update dash timers
	float elapsed_seconds = elapsed_ms_since_last_update / 1000.0f;
	if (is_dashing) {
		dash_timer -= elapsed_seconds;
		if (dash_timer <= 0.0f) {
			is_dashing = false;
			dash_timer = 0.0f;
			dash_cooldown_timer = dash_cooldown;
			dash_direction = {0.0f, 0.0f};
		}
	} else if (dash_cooldown_timer > 0.0f) {
		dash_cooldown_timer -= elapsed_seconds;
		if (dash_cooldown_timer < 0.0f) {
			dash_cooldown_timer = 0.0f;
		}
	}

	bool player_controls_disabled = is_camera_locked_on_bonfire || is_camera_lerping_to_bonfire;

	bool is_moving = false;
	if (!player_controls_disabled) {
		// Update knockback timers
		if (is_knockback) {
			knockback_timer -= elapsed_seconds;
			if (knockback_timer <= 0.0f) {
				is_knockback = false;
				knockback_timer = 0.0f;
				knockback_direction = {0.0f, 0.0f}; // reset knockback direction
			}
		}

		// update hurt knockback timers
		if (is_hurt_knockback) {
			hurt_knockback_timer -= elapsed_seconds;
			if (hurt_knockback_timer <= 0.0f) {
				is_hurt_knockback = false;
				hurt_knockback_timer = 0.0f;
				hurt_knockback_direction = {0.0f, 0.0f}; // reset knockback direction
				
				// resume previous animation after hurt knockback ends
				if (registry.sprites.has(player_salmon)) {
					Sprite& sprite = registry.sprites.get(player_salmon);
					sprite.current_animation = animation_before_hurt;
					if (animation_before_hurt == TEXTURE_ASSET_ID::PLAYER_MOVE) {
						sprite.total_frame = sprite.move_frames;
					} else {
						sprite.total_frame = sprite.idle_frames;
					}
					
					sprite.curr_frame = 0;
					sprite.step_seconds_acc = 0.0f;
					if (registry.renderRequests.has(player_salmon)) {
						RenderRequest& render_request = registry.renderRequests.get(player_salmon);
						render_request.used_texture = get_weapon_texture(animation_before_hurt);
					}
				}
			}
		}

		if (is_knockback) {
			// lock velocity to knockback direction
			motion.velocity.x = knockback_direction.x * salmon_vel * knockback_multiplier;
			motion.velocity.y = knockback_direction.y * salmon_vel * knockback_multiplier;
			is_moving = true;
		} else if (is_hurt_knockback) {
			// lock velocity to hurt knockback direction
			motion.velocity.x = hurt_knockback_direction.x * salmon_vel * hurt_knockback_multiplier;
			motion.velocity.y = hurt_knockback_direction.y * salmon_vel * hurt_knockback_multiplier;
			is_moving = true;
		} else if (is_dashing) {
			// lock velocity to dash direction
			motion.velocity.x = dash_direction.x * salmon_vel * dash_multiplier;
			motion.velocity.y = dash_direction.y * salmon_vel * dash_multiplier;
			is_moving = true;
		} else {
			// normal movement
			float current_vel = salmon_vel;

	if (left_pressed && right_pressed) {
				motion.velocity.x = prioritize_right ? current_vel : -current_vel;
				is_moving = true;
	} else if (left_pressed) {
				motion.velocity.x = -current_vel;
				is_moving = true;
	} else if (right_pressed) {
				motion.velocity.x = current_vel;
				is_moving = true;
	} else {
		motion.velocity.x = 0.0f;
	}

	if (up_pressed && down_pressed) {
				motion.velocity.y = prioritize_down ? current_vel : -current_vel;
				is_moving = true;
	} else if (up_pressed) {
				motion.velocity.y = -current_vel;
				is_moving = true;
	} else if (down_pressed) {
				motion.velocity.y = current_vel;
				is_moving = true;
	} else {
		motion.velocity.y = -0.0f;
			}
		}
	} else {
		motion.velocity.x = 0.0f;
		motion.velocity.y = 0.0f;
	}

	// update fire rate cooldown
	if (fire_rate_cooldown > 0.0f) {
		fire_rate_cooldown -= elapsed_ms_since_last_update / 1000.0f;
		if (fire_rate_cooldown < 0.0f) {
			fire_rate_cooldown = 0.0f;
		}
	}
	// handle automatic firing for assault rifle
	if (left_mouse_pressed && !sprite.is_reloading) {
		if (registry.inventories.has(player_salmon)) {
			Inventory& inventory = registry.inventories.get(player_salmon);
			if (registry.weapons.has(inventory.equipped_weapon)) {
				Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
				Player& player = registry.players.get(player_salmon);
				if (weapon.type == WeaponType::ASSAULT_RIFLE && weapon.fire_rate_rpm == 0.0f) {
					weapon.fire_rate_rpm = 600.0f; // fire rate is set
				}
				// looping rifle sound 
				if (weapon.type == WeaponType::ASSAULT_RIFLE && weapon.fire_rate_rpm > 0.0f && player.ammo_in_mag > 0) {
					if (!rifle_sound_playing && audio_system) {
						audio_system->play("rifle_gunshot", true);
						rifle_sound_playing = true;

						rifle_sound_start_time = current_time_seconds; // record when sound started
					}
				} else if (weapon.type == WeaponType::ASSAULT_RIFLE && player.ammo_in_mag == 0) {
					// Stop sound if out of ammo
					if (rifle_sound_playing && audio_system) {
						float sound_elapsed = current_time_seconds - rifle_sound_start_time;
						float min_play_duration = rifle_sound_min_duration / 13.0f; // 1/13 of sound duration (this is for playing single shot sounds)
						if (sound_elapsed >= min_play_duration) {
							audio_system->stop("rifle_gunshot");
							rifle_sound_playing = false;
						}
					}
				}
				
				if (weapon.fire_rate_rpm > 0.0f && fire_rate_cooldown <= 0.0f && player.ammo_in_mag > 0) {
					auto& motion = registry.motions.get(player_salmon);
					float player_diameter = motion.scale.x;
					float bullet_velocity = 750;
					
					// for player render_offset
					float c = cos(motion.angle);
					float s = sin(motion.angle);
					vec2 rotated_render_offset = {
						player.render_offset.x * c - player.render_offset.y * s,
						player.render_offset.x * s + player.render_offset.y * c
					};
					
					// forward offset from player center
					vec2 forward_offset = {
						player_diameter * 0.55f * cos(motion.angle + M_PI_4 * 0.6f),
						player_diameter * 0.55f * sin(motion.angle + M_PI_4 * 0.6f)
					};
					
					vec2 bullet_spawn_pos = motion.position + rotated_render_offset + forward_offset;
					
					float base_angle = motion.angle;
					createBullet(renderer, bullet_spawn_pos,
						{ bullet_velocity * cos(base_angle), bullet_velocity * sin(base_angle) }, weapon.damage);

					Entity muzzle_flash = Entity();
					Motion& flash_motion = registry.motions.emplace(muzzle_flash);
					flash_motion.position = bullet_spawn_pos;
					flash_motion.angle = base_angle;
					Light& flash_light = registry.lights.emplace(muzzle_flash);
					flash_light.is_enabled = true;
					flash_light.cone_angle = 2.8f;
					flash_light.brightness = 8.0f;
					flash_light.range = 500.0f;
					flash_light.light_color = { 1.0f, 0.9f, 0.5f };
					DeathTimer& flash_timer = registry.deathTimers.emplace(muzzle_flash);
					flash_timer.counter_ms = 50.0f;
					
					player.ammo_in_mag = std::max(0, player.ammo_in_mag - 1);
					
					// keep shooting animation active while firing
					if (!sprite.is_shooting) {
						sprite.is_shooting = true;
						sprite.shoot_timer = sprite.shoot_duration;
						sprite.previous_animation = sprite.current_animation;
						sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_SHOOT;
						sprite.total_frame = sprite.shoot_frames;
						sprite.curr_frame = 0;
						sprite.step_seconds_acc = 0.0f;
						auto& render_request = registry.renderRequests.get(player_salmon);
						render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_SHOOT);
					}
					
					// calculate time between shots based on fire rate
					float time_between_shots = 60.0f / weapon.fire_rate_rpm; // seconds per shot
					fire_rate_cooldown = time_between_shots;
				}
			}
		}
	} else {
		// stop rifle sound when mouse is released
		if (rifle_sound_playing && audio_system) {
			float sound_elapsed = current_time_seconds - rifle_sound_start_time;
			float min_play_duration = rifle_sound_min_duration / 13.0f; // 1/13 of sound duration
			if (sound_elapsed >= min_play_duration) {
				audio_system->stop("rifle_gunshot");
				rifle_sound_playing = false;
			}
		}
	}

	// handle hurt animation
	if (is_hurt_knockback) {
		// show hurt sprite during knockback
		render_request.used_texture = get_hurt_texture();
		sprite.curr_frame = 0; // single frame sprite
		sprite.total_frame = 1;
	} else if (sprite.is_shooting) {
		sprite.shoot_timer -= elapsed_ms_since_last_update / 1000.0f;
		
		bool is_auto_firing = false;
		if (left_mouse_pressed && !sprite.is_reloading) {
			if (registry.inventories.has(player_salmon)) {
				Inventory& inventory = registry.inventories.get(player_salmon);
				if (registry.weapons.has(inventory.equipped_weapon)) {
					Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
					Player& player = registry.players.get(player_salmon);
					if (weapon.fire_rate_rpm > 0.0f && player.ammo_in_mag > 0) {
						is_auto_firing = true;
					}
				}
			}
		}
		
		if (sprite.shoot_timer <= 0.0f) {
			// keep shooting animation active
			if (is_auto_firing) {
				sprite.shoot_timer = sprite.shoot_duration;
			} else {
				// return to previous state
				sprite.is_shooting = false;
				sprite.current_animation = sprite.previous_animation;
				
				if (sprite.previous_animation == TEXTURE_ASSET_ID::PLAYER_MOVE) {
					sprite.total_frame = sprite.move_frames;
				} else {
					sprite.total_frame = sprite.idle_frames;
				}
				
				sprite.curr_frame = 0; // reset animation
				sprite.step_seconds_acc = 0.0f; // reset timer
				render_request.used_texture = get_weapon_texture(sprite.previous_animation);
			}
		}
	}

	// reload animation (skip if hurt)
	if (!is_hurt_knockback && sprite.is_reloading) {
		sprite.reload_timer -= elapsed_ms_since_last_update / 1000.0f;
		if (sprite.reload_timer <= 0.0f) {
			// finish reload
			sprite.is_reloading = false;
			// refill ammo
			if (registry.players.has(player_salmon)) {
				Player& player = registry.players.get(player_salmon);
				player.ammo_in_mag = player.magazine_size;
			}
			// restore animation
			sprite.current_animation = sprite.previous_animation;
			if (sprite.previous_animation == TEXTURE_ASSET_ID::PLAYER_MOVE) {
				sprite.total_frame = sprite.move_frames;
			} else {
				sprite.total_frame = sprite.idle_frames;
			}
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = get_weapon_texture(sprite.previous_animation);
		}
	}
	
    if (!is_hurt_knockback && !sprite.is_shooting && !sprite.is_reloading) {
		if (is_moving && sprite.current_animation != TEXTURE_ASSET_ID::PLAYER_MOVE) {
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_MOVE;
			sprite.total_frame = sprite.move_frames;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_MOVE);
		} else if (!is_moving && sprite.current_animation != TEXTURE_ASSET_ID::PLAYER_IDLE) {
			sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;
			sprite.total_frame = sprite.idle_frames;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_IDLE);
		}
	}
	
	// feet position follow player
	vec2 feet_offset = { 0.f, 5.f };
	float c = cos(motion.angle), s = sin(motion.angle);
	vec2 feet_rotated = { feet_offset.x * c - feet_offset.y * s,
						  feet_offset.x * s + feet_offset.y * c };
	feet_motion.position = motion.position + feet_rotated;
	feet_motion.angle = motion.angle;

	// dash position follow player
	if (is_dashing) {
		// offset dash sprite behind player along dash direction
		vec2 dash_offset = {
			-dash_direction.x * dash_sprite_offset,
			-dash_direction.y * dash_sprite_offset
		};
		vec2 side_offset = {
			-dash_direction.y * dash_sprite_side_offset,
			dash_direction.x * dash_sprite_side_offset
		};
		
		dash_motion.position = motion.position + feet_rotated + dash_offset + side_offset;
		
		// make dash sprite visible
		dash_render_request.used_texture = TEXTURE_ASSET_ID::DASH;
		Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
		dash_motion.scale = mesh.original_size * 90.f;
		// rotate dash sprite to match dash direction
		dash_motion.angle = atan2(dash_direction.y, dash_direction.x);
	} else {
		// hide dash sprite by setting scale to 0
		dash_motion.position = motion.position + feet_rotated;
		dash_motion.scale = {0.0f, 0.0f};
		dash_motion.angle = motion.angle;
	}

	// flashlight position follow player
	vec2 menu_flashlight_offset = {0.f, 0.f};
	if (start_menu_active && !start_menu_transitioning && !start_camera_lerping) {
		menu_flashlight_offset = { window_width_px * 0.28f, window_height_px * 0.12f };
	}
	sync_flashlight_to_player(motion, flashlight_motion, menu_flashlight_offset);

    // use walk spritesheet, pause when not moving
    feet_render_request.used_texture = TEXTURE_ASSET_ID::FEET_WALK;
    feet_sprite.total_frame = 20;
    if (is_moving) {
        feet_sprite.animation_speed = 10.0f;
    } else {
        feet_sprite.animation_speed = 0.0f; // pause at current frame
    }
	
	if (is_player_angle_lerping) {
		player_angle_lerp_time += elapsed_ms_since_last_update;
		float t = player_angle_lerp_time / PLAYER_ANGLE_LERP_DURATION;
		if (t >= 1.0f) {
			t = 1.0f;
			is_player_angle_lerping = false;
		}
		t = t * t * (3.0f - 2.0f * t);
		float current_angle = player_angle_lerp_start + (player_angle_lerp_target - player_angle_lerp_start) * t;
		motion.angle = current_angle;
	}
	else if (!player_controls_disabled) {
		vec2 world_mouse_pos;
		world_mouse_pos.x = mouse_pos.x - (window_width_px / 2.0f) + motion.position.x;
		world_mouse_pos.y = mouse_pos.y - (window_height_px / 2.0f) + motion.position.y;

		vec2 direction = world_mouse_pos - motion.position;
		float angle = atan2(direction.y, direction.x);
		motion.angle = angle;
	}

	// Remove entities that leave the screen on any side
	for (int i = (int)motions_registry.components.size()-1; i>=0; --i) {
		Entity entity = motions_registry.entities[i];
		
		if(registry.players.has(entity)) continue;
		if(registry.obstacles.has(entity)) continue;
		
    // Check all screen boundaries
		/*if (motion.position.x + entity_half_width < screen_left ||
			motion.position.x - entity_half_width > screen_right ||
			motion.position.y + entity_half_height < screen_top ||
			motion.position.y - entity_half_height > screen_bottom) {
			registry.remove_all_components_of(entity);
		}*/
	}

	// Processing the salmon state
	assert(registry.screenStates.components.size() <= 1);
    ScreenState &screen = registry.screenStates.components[0];

    float min_counter_ms = 3000.f;
	for (Entity entity : registry.deathTimers.entities) {
		// progress timer
		DeathTimer& counter = registry.deathTimers.get(entity);
		counter.counter_ms -= elapsed_ms_since_last_update;
		if(counter.counter_ms < min_counter_ms){
		    min_counter_ms = counter.counter_ms;
		}

		if (counter.counter_ms < 0) {
			registry.deathTimers.remove(entity);
			if (registry.players.has(entity)) {
			screen.darken_screen_factor = 0;
            restart_game();
			return true;
			} else {
				registry.remove_all_components_of(entity);
			}
		}
	}
	// reduce window brightness if the salmon is dying
	screen.darken_screen_factor = 1 - min_counter_ms / 3000;

	// despawn bullets that are 2 screen widths away
	float max_bullet_distance = 2.0f * window_width_px;
	float max_distance_squared = max_bullet_distance * max_bullet_distance;
	vec2 player_pos = motion.position;
	
	for (Entity bullet_entity : registry.bullets.entities) {
		if (registry.motions.has(bullet_entity)) {
			Motion& bullet_motion = registry.motions.get(bullet_entity);
			vec2 diff = bullet_motion.position - player_pos;
			float distance_squared = diff.x * diff.x + diff.y * diff.y;
			
			if (distance_squared > max_distance_squared) {
				registry.remove_all_components_of(bullet_entity);
			}
		}
	}

	auto& cooldowns = registry.damageCooldowns;
	for (uint i = 0; i < cooldowns.components.size(); i++) {
		DamageCooldown& cooldown = cooldowns.components[i];
		if (cooldown.cooldown_ms > 0) {
			cooldown.cooldown_ms -= elapsed_ms_since_last_update;
		}
	}

	if (stats_system && registry.players.has(player_salmon)) {
		stats_system->update_player_stats(player_salmon);
	}
	
	survival_time_ms += elapsed_ms_since_last_update;
	
	float SPAWN_RADIUS = level_manager.get_spawn_radius();
	int circle_count = level_manager.get_circle_count();
	
	// Get spawn position for current circle
	// Circle 0 is centered at initial_spawn_position
	// Circle N (N > 0) is centered at the bonfire from circle N-1
	vec2 spawn_position;
	if (circle_count == 0) {
		// Circle 0 uses initial spawn position
		if (circle_bonfire_positions.size() > 0) {
			spawn_position = circle_bonfire_positions[0]; // Initial spawn position
		} else {
			spawn_position = { window_width_px/2.0f, window_height_px - 200.0f };
		}
	} else if (circle_count < circle_bonfire_positions.size()) {
		// Circle N is centered on bonfire from circle N-1 (stored at index N)
		spawn_position = circle_bonfire_positions[circle_count];
	} else if (circle_bonfire_positions.size() > 0) {
		// Fallback to last known position
		spawn_position = circle_bonfire_positions.back();
	} else {
		// Fallback to default if not initialized
		spawn_position = { window_width_px/2.0f, window_height_px - 200.0f };
	}
	
	if (objectives_system) {
		// Update circle level display
		objectives_system->set_circle_level(circle_count);
		
		float required_survival_seconds = level_manager.get_required_survival_time_seconds();
		float survival_seconds = survival_time_ms / 1000.0f;
		bool survival_complete = survival_seconds >= required_survival_seconds;
		char survival_text[64];
		snprintf(survival_text, sizeof(survival_text), "Survival: %.0fs / %.0fs", survival_seconds, required_survival_seconds);
		objectives_system->set_objective(1, survival_complete, survival_text);
		
		int required_kills = level_manager.get_required_kill_count();
		bool kill_complete = kill_count >= required_kills;
		char kill_text[64];
		snprintf(kill_text, sizeof(kill_text), "Kill: %d / %d", kill_count, required_kills);
		objectives_system->set_objective(2, kill_complete, kill_text);
		
		Motion& player_motion = registry.motions.get(player_salmon);
		vec2 diff = player_motion.position - spawn_position;
		float distance_from_spawn = sqrt(diff.x * diff.x + diff.y * diff.y);
		bool exit_radius_complete = distance_from_spawn > SPAWN_RADIUS;
		objectives_system->set_objective(3, exit_radius_complete, "Exit spawn radius");
	}
	
	// Check if both objectives are complete and spawn/reactivate bonfire
	if (registry.players.has(player_salmon)) {
		float required_survival_seconds = level_manager.get_required_survival_time_seconds();
		float survival_seconds = survival_time_ms / 1000.0f;
		bool survival_complete = survival_seconds >= required_survival_seconds;
		int required_kills = level_manager.get_required_kill_count();
		bool kill_complete = kill_count >= required_kills;
		
		// Spawn new bonfire when both objectives are complete
		if (survival_complete && kill_complete) {
			if (!bonfire_spawned) {
				// Spawn new bonfire slightly outside the spawn radius circle
				// Generate random angle for position (0 to 2π)
				float random_angle = uniform_dist(rng) * 2.0f * M_PI;
				
				// Calculate position slightly outside the circumference (15% beyond the radius)
				float bonfire_distance = SPAWN_RADIUS * 1.15f;
				vec2 bonfire_pos = spawn_position + vec2(
					cos(random_angle) * bonfire_distance,
					sin(random_angle) * bonfire_distance
				);
				
				bonfire_entity = createBonfire(renderer, bonfire_pos);
				bonfire_exists = true;
				bonfire_spawned = true;
				
				// Note: Bonfire position will be stored when interacted with (for next circle's center)
				// We don't store it here to avoid overwriting the current circle's center position
				
				std::cerr << "Bonfire spawned at (" << bonfire_pos.x << ", " << bonfire_pos.y << ") after objectives completed" << std::endl;
				
				// TODO: Arrow spawning temporarily disabled
				// Create arrow to point toward bonfire
				// arrow_entity = createArrow(renderer);
				// arrow_exists = true;
				
				// Debug: Verify arrow is not a player
				// if (registry.players.has(arrow_entity)) {
				// 	std::cerr << "ERROR: Arrow entity was created with player component!" << std::endl;
				// 	registry.players.remove(arrow_entity);
				// }
				// if (arrow_entity == player_salmon) {
				// 	std::cerr << "ERROR: Arrow entity is the same as player_salmon!" << std::endl;
				// }
				// std::cerr << "Arrow created: entity=" << arrow_entity << ", player_salmon=" << player_salmon << std::endl;
			}
			// Note: Old disabled bonfires are not reactivated - they stay disabled
		}
	}
	
	// Track player radius state (for other systems that might need it)
	if (registry.players.has(player_salmon)) {
		Motion& player_motion = registry.motions.get(player_salmon);
		vec2 diff = player_motion.position - spawn_position;
		float distance_from_spawn = sqrt(diff.x * diff.x + diff.y * diff.y);
		bool currently_in_radius = distance_from_spawn <= SPAWN_RADIUS;
		player_was_in_radius = currently_in_radius;
	}
	
	if (minimap_system && registry.players.has(player_salmon)) {
		minimap_system->update_player_position(player_salmon, SPAWN_RADIUS, spawn_position, circle_count, circle_bonfire_positions);
		
		// Find active bonfire entity and update minimap (only show active bonfires, not disabled ones)
		vec2 bonfire_pos = {0.0f, 0.0f};
		bool bonfire_found = false;
		for (Entity entity : registry.renderRequests.entities) {
			if (registry.renderRequests.has(entity) && registry.motions.has(entity)) {
				RenderRequest& req = registry.renderRequests.get(entity);
				// Only show active bonfires on minimap (not disabled ones)
				if (req.used_texture == TEXTURE_ASSET_ID::BONFIRE) {
					Motion& bonfire_motion = registry.motions.get(entity);
					bonfire_pos = bonfire_motion.position;
					bonfire_found = true;
					break;
				}
			}
		}
		minimap_system->update_bonfire_position(bonfire_found ? bonfire_pos : vec2(0.0f, 0.0f), SPAWN_RADIUS, spawn_position);
	}
	
	
	if (currency_system && registry.players.has(player_salmon)) {
		Player& player = registry.players.get(player_salmon);
		currency_system->update_currency(player.currency);
	}
	
	// Update level display
	update_level_display();
	
	// Update level transition countdown
	if (is_level_transitioning) {
		level_transition_timer -= elapsed_ms_since_last_update / 1000.0f;
		if (level_transition_timer <= 0.0f) {
			complete_level_transition();
		} else {
			update_level_transition_countdown();
		}
	}

	// Update bonfire instructions visibility (hide during level transition)
	if (!is_level_transitioning) {
		update_bonfire_instructions();
	} else {
		// Hide bonfire instructions during level transition
		if (is_near_bonfire) {
			hide_bonfire_instructions();
		}
	}

	// update visible chunks
	float chunk_size = (float) CHUNK_CELL_SIZE * CHUNK_CELLS_PER_ROW;
	float buffer = 64;
	vec4 cam_view = renderer->getCameraView();

	short left_chunk = (short) std::floor((cam_view.x - buffer) / chunk_size);
	short right_chunk = (short) std::floor((cam_view.y + buffer) / chunk_size);
	short top_chunk = (short) std::floor((cam_view.z - buffer) / chunk_size);
	short bottom_chunk = (short) std::floor((cam_view.w + buffer) / chunk_size);
	for (short i = left_chunk; i <= right_chunk; i++) {
		for (short j = top_chunk; j <= bottom_chunk; j++) {
			if (!registry.chunks.has(i, j)) {
				generateChunk(renderer, vec2(i, j), map_perlin, rng, false);
			}
		}
	}

	// remove off-screen chunks to save space and compute time
	float left_buff_bound = (cam_view.x - 2*buffer);
	float right_buff_bound = (cam_view.y + 2*buffer);
	float top_buff_bound = (cam_view.z - 2*buffer);
	float bottom_buff_bound = (cam_view.w + 2*buffer);

	std::vector<vec2> chunksToRemove;
	for (int i = 0; i < registry.chunks.size(); i++) {
		Chunk& chunk = registry.chunks.components[i];
		short chunk_pos_x = registry.chunks.position_xs[i];
		short chunk_pos_y = registry.chunks.position_ys[i];

		float min_pos_x = (float) registry.chunks.position_xs[i] * chunk_size;
		float max_pos_x = min_pos_x + chunk_size;
		float min_pos_y = (float) registry.chunks.position_ys[i] * chunk_size;
		float max_pos_y = min_pos_y + chunk_size;

		if (max_pos_x <= left_buff_bound || min_pos_x >= right_buff_bound
			|| max_pos_y <= top_buff_bound || min_pos_y >= bottom_buff_bound)
		{
			// serialize chunk + remove entities
			if (!registry.serial_chunks.has(chunk_pos_x, chunk_pos_y)) {
				SerializedChunk& serial_chunk = registry.serial_chunks.emplace(chunk_pos_x, chunk_pos_y);
				for (Entity e : chunk.persistent_entities) {
					if (!registry.motions.has(e))
						continue;

					Motion& e_motion = registry.motions.get(e);
					SerializedTree serial_tree;
					serial_tree.position = e_motion.position;
					serial_tree.scale = e_motion.scale.x;
					serial_chunk.serial_trees.push_back(serial_tree);
				}
			}
			
			for (Entity e : chunk.persistent_entities) {
				// Don't remove bonfire if it's in this chunk (bonfire should persist)
				if (bonfire_exists && e == bonfire_entity) {
					continue;
				}
				registry.remove_all_components_of(e);
			}
			chunksToRemove.push_back(vec2(chunk_pos_x, chunk_pos_y));
		}
	}
	for (vec2 chunk_coord : chunksToRemove) {
		registry.chunks.remove((short) chunk_coord.x, (short) chunk_coord.y);
	}

	spawn_enemies(elapsed_seconds);

	// Update arrow to point toward bonfire (runs at the end, after all other updates)
	// This ensures the arrow position is set after camera position is finalized
	// IMPORTANT: Arrow must NOT be affected by player controls - it is static at screen center
	if (arrow_exists && registry.motions.has(arrow_entity) && registry.arrows.has(arrow_entity)) {
		// Ensure arrow is not a player entity (safety check)
		assert(!registry.players.has(arrow_entity) && "Arrow entity should not have player component!");
		
		// Ensure arrow doesn't have steering or enemy components that might set velocity
		if (registry.enemy_steerings.has(arrow_entity)) {
			registry.enemy_steerings.remove(arrow_entity);
		}
		if (registry.enemies.has(arrow_entity)) {
			registry.enemies.remove(arrow_entity);
		}
		
		Motion& arrow_motion = registry.motions.get(arrow_entity);
		
		// Debug: Check if velocity was set by another system
		if (arrow_motion.velocity.x != 0.f || arrow_motion.velocity.y != 0.f) {
			std::cerr << "WARNING: Arrow velocity was set to (" << arrow_motion.velocity.x 
			          << ", " << arrow_motion.velocity.y << ") before final reset!" << std::endl;
		}
		
		// Position arrow at camera position (screen center) every frame
		vec2 camera_pos = renderer->getCameraPosition();
		vec2 old_arrow_pos = arrow_motion.position;
		arrow_motion.position = camera_pos;
		
		// Debug: Log if arrow position changes unexpectedly
		static vec2 last_camera_pos = {0.f, 0.f};
		if (abs(camera_pos.x - last_camera_pos.x) > 0.1f || abs(camera_pos.y - last_camera_pos.y) > 0.1f) {
			std::cerr << "Arrow: camera_pos=(" << camera_pos.x << ", " << camera_pos.y 
			          << "), arrow_pos=(" << arrow_motion.position.x << ", " << arrow_motion.position.y 
			          << "), old_arrow_pos=(" << old_arrow_pos.x << ", " << old_arrow_pos.y << ")" << std::endl;
			last_camera_pos = camera_pos;
		}
		
		// CRITICAL: Force velocity to zero - arrow must be completely static
		// This must be done here AFTER all other systems that might set velocity
		arrow_motion.velocity = { 0.f, 0.f };
		
		// Update angle to point toward bonfire from player position
		vec2 player_pos = {0.0f, 0.0f};
		vec2 bonfire_pos = {0.0f, 0.0f};
		bool player_found = false;
		bool bonfire_found = false;
		
		// Get player position
		if (registry.players.has(player_salmon) && registry.motions.has(player_salmon)) {
			Motion& player_motion = registry.motions.get(player_salmon);
			player_pos = player_motion.position;
			player_found = true;
		}
		
		// Get bonfire position - search through all motions to find bonfire
		Entity found_bonfire_entity = Entity();
		for (Entity bonfire_search_entity : registry.motions.entities) {
			if (registry.renderRequests.has(bonfire_search_entity)) {
				RenderRequest& req = registry.renderRequests.get(bonfire_search_entity);
				if (req.used_texture == TEXTURE_ASSET_ID::BONFIRE || req.used_texture == TEXTURE_ASSET_ID::BONFIRE_OFF) {
					Motion& bonfire_motion = registry.motions.get(bonfire_search_entity);
					bonfire_pos = bonfire_motion.position;
					found_bonfire_entity = bonfire_search_entity;
					bonfire_found = true;
					break;
				}
			}
		}
		
		if (player_found && bonfire_found) {
			// Check if bonfire is visible on screen - if so, despawn arrow
			vec4 cam_view = renderer->getCameraView();
			float bonfire_radius = 100.0f; // Default radius
			// Get actual bonfire radius from collision circle if available
			if (registry.collisionCircles.has(found_bonfire_entity)) {
				bonfire_radius = registry.collisionCircles.get(found_bonfire_entity).radius;
			}
			
			bool bonfire_on_screen = 
				bonfire_pos.x + bonfire_radius >= cam_view.x && // left bound
				bonfire_pos.x - bonfire_radius <= cam_view.y && // right bound
				bonfire_pos.y + bonfire_radius >= cam_view.z && // top bound
				bonfire_pos.y - bonfire_radius <= cam_view.w;   // bottom bound
			
			if (bonfire_on_screen) {
				// Bonfire is visible on screen - remove arrow
				registry.remove_all_components_of(arrow_entity);
				arrow_exists = false;
			} else {
				// Calculate direction vector from player to bonfire
				vec2 direction = bonfire_pos - player_pos;
				float direction_length = sqrt(direction.x * direction.x + direction.y * direction.y);
				
				// Only update angle if direction is valid (non-zero length)
				if (direction_length > 0.001f) {
					float angle_to_bonfire = atan2(direction.y, direction.x);
					
					// Arrow image's forward direction is upper-right (45 degrees from positive x-axis)
					// Upper-right is at angle -PI/4 (or 7*PI/4), so we need to add PI/4 to align correctly
					arrow_motion.angle = angle_to_bonfire + M_PI / 4.0f;
				}
			}
		}
	}

	return true;
}

void WorldSystem::spawn_enemies(float elapsed_seconds) {\
	if(is_camera_locked_on_bonfire) return;

	spawn_timer += elapsed_seconds;
	wave_timer += elapsed_seconds;

	if (spawn_timer < 3.0f) return;

	spawn_timer = 0.0f;

	if (wave_timer >= 10.0f) {
		wave_count++;
		wave_timer = 0.0f;
	}

	size_t current_enemy_count = registry.enemies.entities.size();

	const size_t MAX_ENEMIES = 25;
	if (current_enemy_count >= MAX_ENEMIES)
			return;

	Motion& player_motion = registry.motions.get(player_salmon);
	int num_enemies = std::min((1 << (wave_count)), (int)(MAX_ENEMIES - current_enemy_count));

	float margin = 50.f;
	for (int i = 0; i < num_enemies; i++) {
		int side = rand() % 4;
		float x, y;
		switch (side) {
			case 0: // left
				x = player_motion.position.x - (window_width_px / 2) - margin;
				y = player_motion.position.y - (window_height_px / 2) + rand() % window_height_px;
				break;
			case 1: // right
				x = player_motion.position.x + (window_width_px / 2) + margin;
				y = player_motion.position.y - (window_height_px / 2) + rand() % window_height_px;
				break;
			case 2: // top
				x = player_motion.position.x - (window_width_px / 2) + rand() % window_width_px;
				y = player_motion.position.y - (window_height_px / 2) - margin;
				break;
			case 3: // bottom
				x = player_motion.position.x - (window_width_px / 2) + rand() % window_width_px;
				y = player_motion.position.y + (window_height_px / 2) + margin;
				break;
		}

		glm::vec2 spawn_pos = {x, y};

		int type = rand() % 3;
		if (type == 0)
			createEnemy(renderer, spawn_pos);
		else if (type == 1) {
			spawn_pos = {
				player_motion.position.x - (window_width_px / 2) - margin,
				player_motion.position.y - (window_height_px / 2) + rand() % window_height_px
			};				
			createSlime(renderer, spawn_pos);
		}	else
			createEvilPlant(renderer, spawn_pos);
	}
}

// Reset the world state to its initial state
void WorldSystem::restart_game() {
	current_speed = 1.f;
	survival_time_ms = 0.f;
	current_time_seconds = 0.0f;
	kill_count = 0;
	player_was_in_radius = true;
	bonfire_spawned = false; // Reset bonfire spawn flag
	is_camera_lerping_to_bonfire = false;
	is_camera_locked_on_bonfire = false;
	is_player_angle_lerping = false;
	should_open_inventory_after_lerp = false;
	arrow_exists = false; // Reset arrow flag
	level_manager.reset(); // Reset level manager to initial state
	
	// Reset bonfire position tracking
	circle_bonfire_positions.clear();
	initial_spawn_position = { window_width_px/2.0f, window_height_px - 200.0f };
	circle_bonfire_positions.push_back(initial_spawn_position); // Circle 0 uses initial spawn position

	if (stats_system) {
		stats_system->set_visible(false);
	}
	if (minimap_system) {
		minimap_system->set_visible(false);
	}
	if (currency_system) {
		currency_system->set_visible(false);
	}
	if (objectives_system) {
		objectives_system->set_visible(false);
	}
	if (menu_icons_system) {
		menu_icons_system->set_visible(false);
	}

	// re-seed perlin noise generators
	unsigned int max_seed = ((((unsigned int) (1 << 31) - 1) << 1) + 1); // 2^32 - 1
	unsigned int map_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
	unsigned int decorator_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
	map_perlin.init(map_seed, 4);
	decorator_perlin.init(decorator_seed, 4);
	printf("Generated seeds: %zi and %zi\n", map_seed, decorator_seed);

	// reset spawn system
	spawn_timer = 0.0f;
	wave_timer = 0.0f;
	wave_count = 0;
	current_level = 1;
	update_level_display();

	// Remove all entities that we created
	// All that have a motion
	while (registry.motions.entities.size() > 0)
	    registry.remove_all_components_of(registry.motions.entities.back());
	registry.serial_chunks.clear();
	registry.chunks.clear();
	
	// Clear inventories (since player entity will be recreated with new ID)
	registry.inventories.clear();

	// create feet first so it renders under the player
	player_feet = createFeet(renderer, { window_width_px/2, window_height_px - 200 }, Entity());

	// create dash sprite
	player_dash = createDash(renderer, { window_width_px/2, window_height_px - 200 }, Entity());

	// create a new Player
	player_salmon = createPlayer(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(player_salmon, {1, 0.8f, 0.8f});
	registry.damageCooldowns.emplace(player_salmon); // Add damage cooldown to player

	// set parent reference now that player exists
	registry.feet.get(player_feet).parent_player = player_salmon;
	registry.feet.get(player_dash).parent_player = player_salmon;

	flashlight = createFlashlight(renderer, { window_width_px/2, window_height_px - 200 });
	registry.colors.insert(flashlight, {1, 1, 1});

	Light& flashlight_light = registry.lights.get(flashlight);
	flashlight_light.follow_target = player_salmon;

	if (registry.motions.has(player_salmon)) {
		Motion& player_motion = registry.motions.get(player_salmon);
		if (!gameplay_started && start_menu_system) {
			vec2 start_screen_offset = { window_width_px * 0.28f, window_height_px * 0.12f };
			start_menu_camera_focus = {
				player_motion.position.x - start_screen_offset.x,
				player_motion.position.y - start_screen_offset.y
			};

			if (renderer) {
				renderer->setCameraPosition(start_menu_camera_focus);
			}

			if (registry.motions.has(flashlight)) {
				sync_flashlight_to_player(player_motion, registry.motions.get(flashlight), start_screen_offset);
			}
		} else if (renderer) {
			renderer->setCameraPosition(player_motion.position);

			if (registry.motions.has(flashlight)) {
				sync_flashlight_to_player(player_motion, registry.motions.get(flashlight));
			}
		}
	}

	// Initialize player inventory
	if (inventory_system) {
		inventory_system->init_player_inventory(player_salmon);
	}

	// generate spawn chunk
	generateChunk(renderer, vec2(0, 0), map_perlin, rng, true);

	// instead of a constant solid background
	// created a quad that can be affected by the lighting
	background = createBackground(renderer);
	registry.colors.insert(background, {0.1f, 0.1f, 0.1f});

	// TODO: remove hardcoded enemy creates
	// glm::vec2 player_init_position = { window_width_px/2, window_height_px - 200 };
	// createSlime(renderer, { player_init_position.x + 300, player_init_position.y + 150 });
	// createSlime(renderer, { player_init_position.x - 300, player_init_position.y + 150 });
	// createEnemy(renderer, { player_init_position.x + 300, player_init_position.y - 150 });
	// createEnemy(renderer, { player_init_position.x - 300, player_init_position.y - 150 });
	// createEnemy(renderer, { player_init_position.x + 350, player_init_position.y });
	// createEnemy(renderer, { player_init_position.x - 350, player_init_position.y });
	// createSlime(renderer, { player_init_position.x - 100, player_init_position.y - 300 });
	// createEnemy(renderer, { player_init_position.x + 100, player_init_position.y - 300 });

	// createEvilPlant(renderer, { player_init_position.x + 50 , player_init_position.y});
	// createEvilPlant(renderer, { player_init_position.x - 50, player_init_position.y});
}

void WorldSystem::fire_weapon() {
	if (is_camera_locked_on_bonfire || is_camera_lerping_to_bonfire) {
		return;
	}

	auto& motion = registry.motions.get(player_salmon);
	auto& sprite = registry.sprites.get(player_salmon);
	auto& render_request = registry.renderRequests.get(player_salmon);
	Player& player = registry.players.get(player_salmon);
	// check if assault rifle is equipped
	bool is_assault_rifle = false;
	if (registry.inventories.has(player_salmon)) {
		Inventory& inventory = registry.inventories.get(player_salmon);
		if (registry.weapons.has(inventory.equipped_weapon)) {
			Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
			if (weapon.type == WeaponType::ASSAULT_RIFLE && weapon.fire_rate_rpm > 0.0f) {
				is_assault_rifle = true;
			}
		}
	}
	bool can_fire = false;
	if (is_assault_rifle) {
		can_fire = !sprite.is_reloading && player.ammo_in_mag > 0;
	} else {
		can_fire = !sprite.is_shooting && !sprite.is_reloading && player.ammo_in_mag > 0;
	}

	if (can_fire) {
		sprite.is_shooting = true;
		sprite.shoot_timer = sprite.shoot_duration;
		sprite.previous_animation = sprite.current_animation;
		sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_SHOOT;
		sprite.total_frame = sprite.shoot_frames;
		sprite.curr_frame = 0; // Reset animation
		sprite.step_seconds_acc = 0.0f; // Reset timer
		render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_SHOOT);

		float player_diameter = motion.scale.x; // same as width
		float bullet_velocity = 750;

		// for player render_offset
		Player& player = registry.players.get(player_salmon);
		float c = cos(motion.angle);
		float s = sin(motion.angle);
		vec2 rotated_render_offset = {
			player.render_offset.x * c - player.render_offset.y * s,
			player.render_offset.x * s + player.render_offset.y * c
		};
		
		// forward offset from player center
		vec2 forward_offset = {
			player_diameter * 0.55f * cos(motion.angle + M_PI_4 * 0.6f),
			player_diameter * 0.55f * sin(motion.angle + M_PI_4 * 0.6f)
		};
		
		vec2 bullet_spawn_pos = motion.position + rotated_render_offset + forward_offset;

		// use player's facing angle instead of mouse direction
		float base_angle = motion.angle;

		// get weapon damage
		int weapon_damage = 20; // default
		bool is_shotgun = false;
		if (registry.inventories.has(player_salmon)) {
			Inventory& inventory = registry.inventories.get(player_salmon);
			if (registry.weapons.has(inventory.equipped_weapon)) {
				Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
				weapon_damage = weapon.damage;
				if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
					is_shotgun = true;
				}
			}
		}

		// play sound (shotgun or pistol)
		if (audio_system) {
			if (is_shotgun) {
				audio_system->play("shotgun_gunshot");
			} else {
				audio_system->play("gunshot");
			}
		}

		if (is_shotgun) {
			// Shotgun fires 5 bullets
			float spread_angles[] = { -20.0f, -10.0f, 0.0f, 10.0f, 20.0f };
			float deg_to_rad = M_PI / 180.0f;
			for (int i = 0; i < 5; i++) {
				float bullet_angle = base_angle + spread_angles[i] * deg_to_rad;
				createBullet(renderer, bullet_spawn_pos,
					{ bullet_velocity * cos(bullet_angle), bullet_velocity * sin(bullet_angle) }, weapon_damage);
			}
			
			// knockback in opposite direction of shooting
			knockback_direction.x = -cos(base_angle);
			knockback_direction.y = -sin(base_angle);
			float dir_len = sqrtf(knockback_direction.x * knockback_direction.x + 
			                     knockback_direction.y * knockback_direction.y);
			if (dir_len > 0.0001f) {
				knockback_direction.x /= dir_len;
				knockback_direction.y /= dir_len;
			}
			is_knockback = true;
			knockback_timer = knockback_duration;
		} else {
			// pistol/rifle fires 1 bullet
			createBullet(renderer, bullet_spawn_pos,
				{ bullet_velocity * cos(base_angle), bullet_velocity * sin(base_angle) }, weapon_damage);
		}

		Entity muzzle_flash = Entity();
		Motion& flash_motion = registry.motions.emplace(muzzle_flash);
		flash_motion.position = bullet_spawn_pos;
		flash_motion.angle = base_angle;
		Light& flash_light = registry.lights.emplace(muzzle_flash);
		flash_light.is_enabled = true;
		flash_light.cone_angle = 2.8f;
		flash_light.brightness = 8.0f;
		flash_light.range = 500.0f;
		flash_light.light_color = { 1.0f, 0.9f, 0.5f };
		DeathTimer& flash_timer = registry.deathTimers.emplace(muzzle_flash);
		flash_timer.counter_ms = 50.0f;

		player.ammo_in_mag = std::max(0, player.ammo_in_mag - 1);
	}
}

// get texture based on equipped weapon
TEXTURE_ASSET_ID WorldSystem::get_weapon_texture(TEXTURE_ASSET_ID base_texture) const {
	if (registry.inventories.has(player_salmon)) {
		Inventory& inventory = registry.inventories.get(player_salmon);
		if (registry.weapons.has(inventory.equipped_weapon)) {
			Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
			if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_IDLE) return TEXTURE_ASSET_ID::SHOTGUN_IDLE;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_MOVE) return TEXTURE_ASSET_ID::SHOTGUN_MOVE;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_SHOOT) return TEXTURE_ASSET_ID::SHOTGUN_SHOOT;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_RELOAD) return TEXTURE_ASSET_ID::SHOTGUN_RELOAD;
			} else if (weapon.type == WeaponType::ASSAULT_RIFLE) {
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_IDLE) return TEXTURE_ASSET_ID::RIFLE_IDLE;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_MOVE) return TEXTURE_ASSET_ID::RIFLE_MOVE;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_SHOOT) return TEXTURE_ASSET_ID::RIFLE_SHOOT;
				if (base_texture == TEXTURE_ASSET_ID::PLAYER_RELOAD) return TEXTURE_ASSET_ID::RIFLE_RELOAD;
			}
		}
	}
	// Default is pistol textures
	return base_texture;
}

// get hurt texture based on equipped weapon
TEXTURE_ASSET_ID WorldSystem::get_hurt_texture() const {
	if (registry.inventories.has(player_salmon)) {
		Inventory& inventory = registry.inventories.get(player_salmon);
		if (registry.weapons.has(inventory.equipped_weapon)) {
			Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
			if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY) {
				return TEXTURE_ASSET_ID::SHOTGUN_HURT;
			} else if (weapon.type == WeaponType::ASSAULT_RIFLE) {
				return TEXTURE_ASSET_ID::RIFLE_HURT;
			}
		}
	}
	// default
	return TEXTURE_ASSET_ID::PISTOL_HURT;
}

void WorldSystem::play_hud_intro()
{
#ifdef HAVE_RMLUI
	if (stats_system) {
		stats_system->play_intro_animation();
	}
	if (minimap_system) {
		minimap_system->play_intro_animation();
	}
	if (currency_system) {
		currency_system->play_intro_animation();
	}
	if (objectives_system) {
		objectives_system->play_intro_animation();
	}
	if (menu_icons_system) {
		menu_icons_system->play_intro_animation();
	}
	// Level display visibility is managed by CurrencySystem's shared HUD document
#endif
}

void WorldSystem::update_paused(float elapsed_ms)
{
	(void)elapsed_ms;

	if (!(start_menu_active || start_menu_transitioning)) {
		return;
	}

	auto sync_flashlight_now = [this]() {
		if (registry.motions.has(player_salmon) && registry.motions.has(flashlight)) {
			Motion& player_motion = registry.motions.get(player_salmon);
			Motion& flashlight_motion = registry.motions.get(flashlight);

			vec2 camera_pos = renderer ? renderer->getCameraPosition() : player_motion.position;

			vec2 world_mouse_pos;
			world_mouse_pos.x = mouse_pos.x - (window_width_px / 2.0f) + camera_pos.x;
			world_mouse_pos.y = mouse_pos.y - (window_height_px / 2.0f) + camera_pos.y;

			vec2 direction = world_mouse_pos - player_motion.position;
			if (direction.x != 0.f || direction.y != 0.f) {
				player_motion.angle = atan2(direction.y, direction.x);
			}

			sync_flashlight_to_player(player_motion, flashlight_motion);


		}
	};
	sync_flashlight_now();

	if (start_camera_lerping) {
		start_camera_lerp_time += elapsed_ms;
		float t = start_camera_lerp_time / START_CAMERA_LERP_DURATION;
		if (t >= 1.0f) {
			t = 1.0f;
			start_camera_lerping = false;
			finalize_start_menu_transition();
		}
		float smooth = t * t * (3.0f - 2.0f * t);
		vec2 new_pos = start_camera_lerp_start + (start_camera_lerp_target - start_camera_lerp_start) * smooth;
		if (renderer) {
			renderer->setCameraPosition(new_pos);
		}
		sync_flashlight_now();
	} else if (renderer) {
		if (start_menu_transitioning) {
			if (registry.motions.has(player_salmon)) {
				renderer->setCameraPosition(registry.motions.get(player_salmon).position);
			} else {
				renderer->setCameraPosition(start_camera_lerp_target);
			}
			sync_flashlight_now();
			finalize_start_menu_transition();
		} else {
			renderer->setCameraPosition(start_menu_camera_focus);
			sync_flashlight_now();
		}
	}
}

// Compute collisions between entities
void WorldSystem::handle_collisions() {
	// Loop over all collisions detected by the physics system
	auto& collisionsRegistry = registry.collisions;
	for (uint i = 0; i < collisionsRegistry.components.size(); i++) {
		// The entity and its collider
		Entity entity = collisionsRegistry.entities[i];
		Entity entity_other = collisionsRegistry.components[i].other;
		// for now, we are only interested in collisions that involve the salmon
		if (registry.players.has(entity)) {
			//Player& player = registry.players.get(entity);

		}

		// When enemy was shot by the bullet
		if (registry.enemies.has(entity) && registry.bullets.has(entity_other)) {
			Enemy& enemy = registry.enemies.get(entity);
			Motion& enemy_motion = registry.motions.get(entity);
			Bullet& bullet = registry.bullets.get(entity_other);
			Motion& bullet_motion = registry.motions.get(entity_other);

			// Subtract bullet damage from enemy health
			enemy.health -= bullet.damage;
			enemy.is_hurt = true;
			enemy.healthbar_visibility_timer = 3.0f;  // Show healthbar for 3 seconds after taking damage
			if(!registry.stationaryEnemies.has(entity)) enemy_motion.velocity = bullet_motion.velocity * 0.5f;

			// Check if enemy should die
			if (enemy.health <= 0) {
				enemy.is_dead = true;

				// Award xylarite to player
				Player& player = registry.players.get(player_salmon);
				player.currency += 10; // 10 xylarite per enemy

				// Update currency UI
				if (currency_system) {
					currency_system->update_currency(player.currency);
				}
			}

			// Play impact sound
			if (audio_system) {
				audio_system->play("impact-enemy");
			}

			// Destroy the bullet
			registry.remove_all_components_of(entity_other);
		}


		// When player was hit by enemy bullet
		if (registry.players.has(entity) && registry.bullets.has(entity_other) && registry.deadlies.has(entity_other)) {
			Player& player = registry.players.get(player_salmon);

			// Subtract damage from player health
			player.health -= 10.0;

			// calculate knockback direction (away from bullet)
			if (registry.motions.has(entity) && registry.motions.has(entity_other)) {
				Motion& player_motion = registry.motions.get(entity);
				Motion& bullet_motion = registry.motions.get(entity_other);
				vec2 direction = player_motion.position - bullet_motion.position;
				float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y);
				if (dir_len > 0.0001f) {
					hurt_knockback_direction.x = direction.x / dir_len;
					hurt_knockback_direction.y = direction.y / dir_len;
					is_hurt_knockback = true;
					hurt_knockback_timer = hurt_knockback_duration;
					
					// store current animation before hurt
					if (registry.sprites.has(entity)) {
						Sprite& sprite = registry.sprites.get(entity);
						animation_before_hurt = sprite.current_animation;
					}
				}
			}

			// Destroy the bullet
			registry.remove_all_components_of(entity_other);

			// Check if player is dead
			if (player.health <= 0) {
				player.health = 0;
				restart_game();
			}
		}

		// When bullet hits an obstacle (tree)
		if (registry.obstacles.has(entity) && registry.bullets.has(entity_other)) {
			// Play tree impact sound
			if (audio_system) {
				audio_system->play("impact-tree");
			}

			// Destroy the bullet
			registry.remove_all_components_of(entity_other);
		}

		// When player is hit by the enemy
		if (registry.enemies.has(entity) && registry.players.has(entity_other)) {
			// Deplete player's health with cooldown
			if (registry.damageCooldowns.has(entity_other)) {
				DamageCooldown& cooldown = registry.damageCooldowns.get(entity_other);
				if (cooldown.cooldown_ms <= 0) {
				Player& player = registry.players.get(entity_other);
				Enemy& enemy = registry.enemies.get(entity);
				player.health -= enemy.damage;
				cooldown.cooldown_ms = cooldown.max_cooldown_ms;
				
					// calculate knockback direction (away from enemy)
					if (registry.motions.has(entity) && registry.motions.has(entity_other)) {
						Motion& enemy_motion = registry.motions.get(entity);
						Motion& player_motion = registry.motions.get(entity_other);
						vec2 direction = player_motion.position - enemy_motion.position;
						float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y);
						if (dir_len > 0.0001f) {
							hurt_knockback_direction.x = direction.x / dir_len;
							hurt_knockback_direction.y = direction.y / dir_len;
							is_hurt_knockback = true;
							hurt_knockback_timer = hurt_knockback_duration;
							
							// store current animation before hurt
							if (registry.sprites.has(entity_other)) {
								Sprite& sprite = registry.sprites.get(entity_other);
								animation_before_hurt = sprite.current_animation;
							}
						}
					}
					
					// Check if player is dead
					if (player.health <= 0) {
						player.health = 0;
						restart_game();
					}
				}
			}
		}
	}

	// Remove all collisions from this simulation step
	registry.collisions.clear();
}

// Should the game be over ?
bool WorldSystem::is_over() const {
	return bool(glfwWindowShouldClose(window));
}

void WorldSystem::exit_bonfire_mode() {
	// Exit bonfire mode if currently locked on bonfire
	if (is_camera_locked_on_bonfire && registry.players.has(player_salmon)) {
		Motion& player_motion = registry.motions.get(player_salmon);
		is_camera_lerping_to_bonfire = true;
		camera_lerp_start = camera_lerp_target;
		camera_lerp_target = player_motion.position;
		camera_lerp_time = 0.f;
		is_camera_locked_on_bonfire = false;
		should_open_inventory_after_lerp = false; // Cancel inventory opening if exiting bonfire
	}
}

// On key callback
void WorldSystem::on_key(int key, int, int action, int mod) {
	const bool menu_blocking = start_menu_active && !start_menu_transitioning;
	if (start_menu_transitioning && start_menu_system) {
		start_menu_system->on_key(key, action, mod);
	}

	if (menu_blocking) {
		if (start_menu_system) {
			start_menu_system->on_key(key, action, mod);
		}
		return;
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		request_return_to_menu();
		return;
	}

	// track which keys are being pressed, for player movement
	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_S) {
		down_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_down = true;
		}
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_S) {
		down_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_W) {
		up_pressed = true;
		if (action == GLFW_PRESS) {
			prioritize_down = false;
		}
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_W) {
		up_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_A) {
		left_pressed = true;
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_A) {
		left_pressed = false;
	}

	if ((action == GLFW_PRESS || action == GLFW_REPEAT) && key == GLFW_KEY_D) {
		right_pressed = true;
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Move);
	} else if (action == GLFW_RELEASE && key == GLFW_KEY_D) {
		right_pressed = false;
	}

	// Resetting game
	// Keybind moved to "=" to free "R" for reload and ensure keybind is pressable on newer Macs
    if (action == GLFW_RELEASE && key == GLFW_KEY_EQUAL) {
		int w, h;
		glfwGetWindowSize(window, &w, &h);

        restart_game();
	}

	// M3 TEST: regenerate the world
	if (action == GLFW_RELEASE && key == GLFW_KEY_G) {
		// clear chunks and obstacles
		registry.serial_chunks.clear();
		registry.chunks.clear();
		while (!registry.obstacles.entities.empty()) {
			Entity obstacle = registry.obstacles.entities.back();
			registry.remove_all_components_of(obstacle);
		}

		// reseed noise generators
		unsigned int max_seed = ((((unsigned int) (1 << 31) - 1) << 1) + 1);
		unsigned int map_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
		unsigned int decorator_seed = (unsigned int) ((float) max_seed * uniform_dist(rng));
		map_perlin.init(map_seed);
		decorator_perlin.init(decorator_seed);

		// re-generate spawn chunk
		if (registry.motions.has(player_salmon)) {
			Motion& p_motion = registry.motions.get(player_salmon);
			float chunk_size = (float) CHUNK_CELL_SIZE * CHUNK_CELLS_PER_ROW;
			vec2 chunk_pos = vec2(floor(p_motion.position.x / chunk_size), floor(p_motion.position.y / chunk_size));
			generateChunk(renderer, chunk_pos, map_perlin, rng, true);
		}
		
	}

	if (action == GLFW_RELEASE && key == GLFW_KEY_I) {
		/* Inventory can only be accessed at a bonfire
		if (!is_near_bonfire) {
			return;
		}
		*/

		if (tutorial_system && tutorial_system->is_active() && tutorial_system->should_pause()) {
			if (tutorial_system->get_required_action() == TutorialSystem::Action::OpenInventory) {
				tutorial_system->on_next_clicked();
			}
		}
		if (inventory_system) {
			inventory_system->toggle_inventory();
		}
		if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::OpenInventory);
	}

	if (action == GLFW_PRESS && key == GLFW_KEY_E) {
		if (registry.players.has(player_salmon) && !is_camera_lerping_to_bonfire) {
			Motion& player_motion = registry.motions.get(player_salmon);
			
			if (is_camera_locked_on_bonfire) {
				is_camera_lerping_to_bonfire = true;
				camera_lerp_start = camera_lerp_target;
				camera_lerp_target = player_motion.position;
				camera_lerp_time = 0.f;
				is_camera_locked_on_bonfire = false;
				should_open_inventory_after_lerp = false; // Cancel inventory opening if exiting bonfire
				return;
			}
			
			const float INTERACTION_DISTANCE = 100.0f;
			
			for (Entity entity : registry.obstacles.entities) {
				if (!registry.motions.has(entity)) continue;
				
				Motion& bonfire_motion = registry.motions.get(entity);
				
				if (registry.renderRequests.has(entity) && registry.collisionCircles.has(entity)) {
					RenderRequest& req = registry.renderRequests.get(entity);
					// Only allow interaction with active bonfire (not the off state)
					if (req.used_texture == TEXTURE_ASSET_ID::BONFIRE) {
						
						vec2 diff = bonfire_motion.position - player_motion.position;
						float distance = sqrt(diff.x * diff.x + diff.y * diff.y);
						
						float bonfire_radius = registry.collisionCircles.get(entity).radius;
						if (distance < INTERACTION_DISTANCE + bonfire_radius) {
							// Mark all enemies as dead and immediately remove them to prevent
							// delayed kill callbacks from incrementing kill_count after reset
							// Collect all enemy entities first to avoid iteration issues
							std::vector<Entity> enemies_to_remove;
							for (Entity enemy_entity : registry.enemies.entities) {
								if (!registry.enemies.has(enemy_entity)) continue;
								Enemy& enemy = registry.enemies.get(enemy_entity);
								if (!enemy.is_dead) {
									enemy.is_dead = true;
								}
								enemies_to_remove.push_back(enemy_entity);
							}
							// Remove all enemies immediately to prevent their callbacks
							// from firing after we reset kill_count
							for (Entity enemy_entity : enemies_to_remove) {
								registry.remove_all_components_of(enemy_entity);
							}
							
							// Store the bonfire position for the new circle before incrementing circle_count
							int new_circle = level_manager.get_circle_count() + 1;
							if (circle_bonfire_positions.size() <= new_circle) {
								circle_bonfire_positions.resize(new_circle + 1);
							}
							circle_bonfire_positions[new_circle] = bonfire_motion.position;
							
							// Reset objectives immediately when bonfire is interacted with
							// This ensures the reset happens before the inventory opens
							level_manager.start_new_circle();
							survival_time_ms = 0.0f;
							kill_count = 0;
							bonfire_spawned = false; // Allow new bonfire to spawn for next level
							
							// Store the bonfire entity to change its state when inventory opens
							bonfire_entity = entity;
							
							vec2 direction_to_bonfire = bonfire_motion.position - player_motion.position;
							float target_angle = atan2(direction_to_bonfire.y, direction_to_bonfire.x);
							
							is_camera_lerping_to_bonfire = true;
							camera_lerp_start = player_motion.position;
							camera_lerp_target = bonfire_motion.position;
							camera_lerp_time = 0.f;
							
							is_player_angle_lerping = true;
							player_angle_lerp_start = player_motion.angle;
							player_angle_lerp_target = target_angle;
							
							float angle_diff = player_angle_lerp_target - player_angle_lerp_start;
							if (angle_diff > M_PI) {
								angle_diff -= 2.0f * M_PI;
							} else if (angle_diff < -M_PI) {
								angle_diff += 2.0f * M_PI;
							}
							player_angle_lerp_target = player_angle_lerp_start + angle_diff;
							player_angle_lerp_time = 0.f;
							
							// Set flag to open inventory after lerping animations complete
							should_open_inventory_after_lerp = true;
							
							break;
						}
					}
				}
			}
		}
	}

	// Cmd+R (Mac) or Ctrl+R: Hot reload UI (for development)
    if (action == GLFW_RELEASE && key == GLFW_KEY_R && (mod & GLFW_MOD_SUPER || mod & GLFW_MOD_CONTROL)) {
		if (inventory_system && inventory_system->is_inventory_open()) {
			std::cout << "⌘+R pressed - manually reloading UI..." << std::endl;
			inventory_system->reload_ui();
		}
	}

    if (action == GLFW_RELEASE && key == GLFW_KEY_R && !(mod & (GLFW_MOD_SUPER | GLFW_MOD_CONTROL))) {
        if (tutorial_system) tutorial_system->notify_action(TutorialSystem::Action::Reload);
        if (registry.players.has(player_salmon)) {
            Player& player = registry.players.get(player_salmon);
            Sprite& sprite = registry.sprites.get(player_salmon);
            auto& render_request = registry.renderRequests.get(player_salmon);
            if (!sprite.is_reloading && !sprite.is_shooting && player.ammo_in_mag < player.magazine_size) {
                // determine reload frames based on equipped weapon
                int reload_frame_count = sprite.reload_frames; // default to pistol
                if (registry.inventories.has(player_salmon)) {
                    Inventory& inventory = registry.inventories.get(player_salmon);
                    if (registry.weapons.has(inventory.equipped_weapon)) {
                        Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
                        // shotgun and rifle use 20 frames, pistol uses 15
                        if (weapon.type == WeaponType::PLASMA_SHOTGUN_HEAVY || 
                            weapon.type == WeaponType::ASSAULT_RIFLE) {
                            reload_frame_count = 20;
                        }
                    }
                }
                
                sprite.is_reloading = true;
                sprite.reload_timer = sprite.reload_duration;
                sprite.previous_animation = sprite.current_animation;
                sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_RELOAD;
                sprite.total_frame = reload_frame_count;
                sprite.curr_frame = 0;
                sprite.step_seconds_acc = 0.0f;
                render_request.used_texture = get_weapon_texture(TEXTURE_ASSET_ID::PLAYER_RELOAD);
            }
        }
    }

	// Debugging
	if (key == GLFW_KEY_D) {
		if (action == GLFW_RELEASE)
			debugging.in_debug_mode = false;
		else
			debugging.in_debug_mode = true;
	}

	// Toggle ambient brightness with 'O' key
	if (action == GLFW_PRESS && key == GLFW_KEY_O) {
		renderer->setGlobalAmbientBrightness(1.0f - renderer->global_ambient_brightness);
	}

	// Toggle player hitbox debug with 'C'
	if (action == GLFW_RELEASE && key == GLFW_KEY_C) {
		renderer->togglePlayerHitboxDebug();
	}

	// Dash with SHIFT key, only if moving and cooldown is ready
	if (action == GLFW_PRESS && key == GLFW_KEY_LEFT_SHIFT) {
		if (!is_dashing && dash_cooldown_timer <= 0.0f) {
			float dir_x = 0.0f;
			float dir_y = 0.0f;
			
			if (left_pressed && right_pressed) {
				dir_x = prioritize_right ? 1.0f : -1.0f;
			} else if (left_pressed) {
				dir_x = -1.0f;
			} else if (right_pressed) {
				dir_x = 1.0f;
			}
			if (up_pressed && down_pressed) {
				dir_y = prioritize_down ? 1.0f : -1.0f;
			} else if (up_pressed) {
				dir_y = -1.0f;
			} else if (down_pressed) {
				
				dir_y = 1.0f;
			}
			
			// normalize vector
			float dir_len = sqrtf(dir_x * dir_x + dir_y * dir_y);
			if (dir_len > 0.0001f) {
				dash_direction.x = dir_x / dir_len;
				dash_direction.y = dir_y / dir_len;
				is_dashing = true;
				dash_timer = dash_duration;
			}
		}
	}


	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_COMMA) {
		current_speed -= 0.1f;
	}
	if (action == GLFW_RELEASE && (mod & GLFW_MOD_SHIFT) && key == GLFW_KEY_PERIOD) {
		current_speed += 0.1f;
	}
	current_speed = fmax(0.f, current_speed);

	// Handle N key for next level
	if (action == GLFW_PRESS && key == GLFW_KEY_N) {
		if (is_near_bonfire && !is_camera_lerping_to_bonfire) {
			handle_next_level();
		}
	}
}

void WorldSystem::on_mouse_move(vec2 mouse_position) {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// xpos and ypos are relative to the top-left of the window, the salmon's
	// default facing direction is (1, 0)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	mouse_pos = mouse_position;

	const bool menu_blocking = start_menu_active && !start_menu_transitioning;
	if (start_menu_transitioning && start_menu_system) {
		start_menu_system->on_mouse_move(mouse_position);
	}

	if (menu_blocking) {
		if (start_menu_system) {
			start_menu_system->on_mouse_move(mouse_position);
		}
		return;
	}

	// Pass mouse position to tutorial for hover effects
	if (tutorial_system && tutorial_system->is_active()) {
		tutorial_system->on_mouse_move(mouse_position);
	}

	if (menu_icons_system) {
		menu_icons_system->on_mouse_move(mouse_position);
	}

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_move(mouse_position);
	}
}

void WorldSystem::on_mouse_click(int button, int action, int mods) {
	const bool menu_blocking = start_menu_active && !start_menu_transitioning;
	if (start_menu_transitioning && start_menu_system) {
		start_menu_system->on_mouse_button(button, action, mods);
	}

	if (menu_blocking) {
		if (start_menu_system) {
			start_menu_system->on_mouse_button(button, action, mods);
		}
		return;
	}

	// Check menu icons first - they should always be clickable (especially exit button)
	// This allows exiting even when tutorial is active
	if (menu_icons_system && menu_icons_system->on_mouse_button(button, action, mods)) {
		return;
	}

	if (tutorial_system && tutorial_system->should_pause()) {
		tutorial_system->on_mouse_button(button, action, mods);
		return;
	}

	if (tutorial_system && tutorial_system->is_active() && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		tutorial_system->notify_action(TutorialSystem::Action::Shoot);
	}

	if (inventory_system && inventory_system->is_inventory_open()) {
		inventory_system->on_mouse_button(button, action, mods);
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			left_mouse_pressed = true;
			if (is_camera_locked_on_bonfire || is_camera_lerping_to_bonfire) {
				return;
			}
			
			if (registry.inventories.has(player_salmon)) {
				Inventory& inventory = registry.inventories.get(player_salmon);
				if (registry.weapons.has(inventory.equipped_weapon)) {
					Weapon& weapon = registry.weapons.get(inventory.equipped_weapon);
					
					// fire rate is set for assault rifle
					if (weapon.type == WeaponType::ASSAULT_RIFLE && weapon.fire_rate_rpm == 0.0f) {
						weapon.fire_rate_rpm = 600.0f;
					}
					
					if (weapon.fire_rate_rpm > 0.0f) {
						fire_weapon();
						float time_between_shots = 60.0f / weapon.fire_rate_rpm;
						fire_rate_cooldown = time_between_shots;
					} else {
						fire_weapon();
					}
				} else {
					fire_weapon();
				}
			} else {
				fire_weapon();
			}
		} else if (action == GLFW_RELEASE) {
			left_mouse_pressed = false;
			// stop rifle sound when mouse is released
			if (rifle_sound_playing && audio_system) {
				audio_system->stop("rifle_gunshot");
				rifle_sound_playing = false;
			}
		}
	}
}

void WorldSystem::update_bonfire_instructions()
{
#ifdef HAVE_RMLUI
	if (!registry.players.has(player_salmon) || !bonfire_instructions_document) {
		if (is_near_bonfire) {
			hide_bonfire_instructions();
		}
		return;
	}

	Motion& player_motion = registry.motions.get(player_salmon);
	const float INTERACTION_DISTANCE = 2.0f; // Reduced for "really close" requirement
	bool near_any_bonfire = false;
	Entity nearest_bonfire = Entity();

	// Check distance to all bonfires
	for (Entity entity : registry.obstacles.entities) {
		if (!registry.motions.has(entity)) continue;
		
		Motion& bonfire_motion = registry.motions.get(entity);
		
		if (registry.renderRequests.has(entity) && registry.collisionCircles.has(entity)) {
			RenderRequest& req = registry.renderRequests.get(entity);
			if (req.used_texture == TEXTURE_ASSET_ID::BONFIRE) {
				vec2 diff = bonfire_motion.position - player_motion.position;
				float distance = sqrt(diff.x * diff.x + diff.y * diff.y);
				
				float bonfire_radius = registry.collisionCircles.get(entity).radius;
				if (distance < INTERACTION_DISTANCE + bonfire_radius) {
					near_any_bonfire = true;
					nearest_bonfire = entity;
					break;
				}
			}
		}
	}

	// Update UI visibility based on proximity
	if (near_any_bonfire && !is_near_bonfire) {
		current_bonfire_entity = nearest_bonfire;
		show_bonfire_instructions();
		is_near_bonfire = true;
	} else if (!near_any_bonfire && is_near_bonfire) {
		hide_bonfire_instructions();
		is_near_bonfire = false;
		current_bonfire_entity = Entity();
		// Close inventory if player moves away from bonfire
		if (inventory_system && inventory_system->is_inventory_open()) {
			inventory_system->hide_inventory();
		}
	} else if (near_any_bonfire && is_near_bonfire) {
		// Update which bonfire we're tracking if it changed
		current_bonfire_entity = nearest_bonfire;
	}
	
	// Always update position every frame when instructions are visible
	// This is necessary because camera position changes every frame
	if (is_near_bonfire && registry.motions.has(current_bonfire_entity)) {
		update_bonfire_instructions_position();
	}
#endif
}

void WorldSystem::update_bonfire_instructions_position()
{
#ifdef HAVE_RMLUI
	if (!bonfire_instructions_document || !registry.motions.has(current_bonfire_entity) || !renderer) {
		return;
	}

	Motion& bonfire_motion = registry.motions.get(current_bonfire_entity);
	
	// Get camera position from renderer's camera view
	vec4 cam_view = renderer->getCameraView();
	vec2 camera_position;
	camera_position.x = (cam_view.x + cam_view.y) / 2.0f;
	camera_position.y = (cam_view.z + cam_view.w) / 2.0f;
	
	// Calculate instruction position in world space
	vec2 instruction_world_pos;
	instruction_world_pos.x = bonfire_motion.position.x + 150.0f;
	instruction_world_pos.y = bonfire_motion.position.y + 150.0f;
	
	// Convert instruction world position to screen coordinates
	// This makes the text move with the camera, staying fixed over the bonfire
	float screen_x = instruction_world_pos.x - camera_position.x + (window_width_px / 2.0f);
	float screen_y = instruction_world_pos.y - camera_position.y + (window_height_px / 2.0f);
	
	// Center horizontally on the bonfire
	Rml::Element* instructions_box = bonfire_instructions_document->GetElementById("bonfire_instructions");
	if (instructions_box) {
		char left_str[32];
		char top_str[32];
		snprintf(left_str, sizeof(left_str), "%.1fpx", screen_x);
		snprintf(top_str, sizeof(top_str), "%.1fpx", screen_y);
		
		instructions_box->SetProperty("left", left_str);
		instructions_box->SetProperty("top", top_str);
		instructions_box->SetProperty("transform", "translateX(-50%)"); // Center horizontally
	}
#endif
}

void WorldSystem::show_bonfire_instructions()
{
#ifdef HAVE_RMLUI
	if (!bonfire_instructions_document) return;
	
	// Ensure document is shown
	if (!bonfire_instructions_document->IsVisible()) {
		bonfire_instructions_document->Show();
	}
	
	// Update position
	update_bonfire_instructions_position();
	
	Rml::Element* container = bonfire_instructions_document->GetElementById("bonfire_instructions_container");
	if (container) {
		container->SetClass("visible", true);
	}
#endif
}

void WorldSystem::hide_bonfire_instructions()
{
#ifdef HAVE_RMLUI
	if (!bonfire_instructions_document) return;
	
	Rml::Element* container = bonfire_instructions_document->GetElementById("bonfire_instructions_container");
	if (container) {
		container->SetClass("visible", false);
	}
	// Note: We keep the document shown but use CSS class to control visibility
	// This allows transitions to work properly
#endif
}

void WorldSystem::handle_next_level()
{
#ifdef HAVE_RMLUI
	if (is_level_transitioning) {
		return; // Already transitioning
	}
	
	// Show level transition splash screen
	is_level_transitioning = true;
	level_transition_timer = LEVEL_TRANSITION_DURATION;
	
	if (level_transition_document) {
		Rml::Element* container = level_transition_document->GetElementById("level_transition_container");
		if (container) {
			container->SetClass("visible", true);
		}
		
		// Update level title
		Rml::Element* level_title = level_transition_document->GetElementById("level_title");
		if (level_title) {
			char title_str[64];
			snprintf(title_str, sizeof(title_str), "Level %d", current_level + 1);
			level_title->SetInnerRML(title_str);
		}
		
		// Update countdown
		update_level_transition_countdown();
	}
	
	// Hide bonfire instructions when showing level transition
	if (is_near_bonfire) {
		hide_bonfire_instructions();
	}
#endif
}

void WorldSystem::update_level_display()
{
#ifdef HAVE_RMLUI
	// Access level display from the shared HUD document via currency system
	if (!currency_system) {
		return;
	}
	
	// Get the document from currency system (it manages the shared HUD document)
	Rml::ElementDocument* hud_document = currency_system->get_document();
	if (!hud_document) {
		return;
	}
	
	Rml::Element* level_display = hud_document->GetElementById("level_display");
	if (level_display) {
		char level_str[32];
		snprintf(level_str, sizeof(level_str), "Lv. %d", current_level);
		level_display->SetInnerRML(level_str);
	}
#endif
}

void WorldSystem::update_level_transition_countdown()
{
#ifdef HAVE_RMLUI
	if (!level_transition_document) {
		return;
	}
	
	Rml::Element* countdown_text = level_transition_document->GetElementById("countdown_text");
	if (countdown_text) {
		int seconds = (int)ceil(level_transition_timer);
		char countdown_str[64];
		snprintf(countdown_str, sizeof(countdown_str), "Spawning in %d second%s", seconds, seconds == 1 ? "" : "s");
		countdown_text->SetInnerRML(countdown_str);
	}
#endif
}

void WorldSystem::complete_level_transition()
{
#ifdef HAVE_RMLUI
	// Hide splash screen
	if (level_transition_document) {
		Rml::Element* container = level_transition_document->GetElementById("level_transition_container");
		if (container) {
			container->SetClass("visible", false);
		}
	}
	
	is_level_transitioning = false;
#endif
	
	// Progress to next level
	current_level++;
	update_level_display();
	
	// TODO: Add level-specific logic here (difficulty scaling, new areas, etc.)
	// For example: restart_game() with increased difficulty, or load a new level
}
