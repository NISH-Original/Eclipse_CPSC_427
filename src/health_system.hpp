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

private:
};

