#pragma once

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs_registry.hpp"

// System responsible for managing player health
class HealthSystem
{
public:
	HealthSystem();
	
	bool take_damage(Entity player_entity, float damage);
	
	float heal(Entity player_entity, float amount);
	
	void set_health(Entity player_entity, float health);
	
	void set_max_health(Entity player_entity, float max_health);
	
	float get_health(Entity player_entity) const;
	
	float get_max_health(Entity player_entity) const;
	
	float get_health_percent(Entity player_entity) const;
	
	bool is_dead(Entity player_entity) const;
	
	bool is_full_health(Entity player_entity) const;
	
	Entity get_player_entity() const;
	
	bool has_player() const;
	
	void update(float elapsed_ms);
	
	void set_heal_delay_ms(float delay_ms);
	
	float get_heal_delay_ms() const;
	
	void set_heal_rate(float health_per_second);
	
	float get_heal_rate() const;
	
	void reset_healing_timer();
	
	// Check if healing is currently active
	bool is_healing(Entity player_entity) const;

private:
	// Time since last damage was taken
	float time_since_last_damage_ms = 0.0f;
	
	// Delay before healing starts after taking damage
	float heal_delay_ms = 30000.0f;
	
	// Heal rate in health points per second
	float heal_rate = 10.0f;
	
	// Track if healing has started (once started, continues until full health)
	bool healing_active = false;
};

