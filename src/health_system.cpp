#include "health_system.hpp"
#include <algorithm>

HealthSystem::HealthSystem()
{
}

bool HealthSystem::take_damage(Entity player_entity, float damage)
{
	if (!registry.players.has(player_entity)) {
		return false;
	}
	
	Player& player = registry.players.get(player_entity);
	
	player.health -= damage;
	
	player.health = std::max(0.0f, player.health);
	
	time_since_last_damage_ms = 0.0f;
	
	return player.health <= 0.0f;
}

float HealthSystem::heal(Entity player_entity, float amount)
{
	if (!registry.players.has(player_entity)) {
		return 0.0f;
	}
	
	Player& player = registry.players.get(player_entity);
	
	float old_health = player.health;
	player.health = std::min(player.max_health, player.health + amount);
	
	return player.health - old_health;
}

void HealthSystem::set_health(Entity player_entity, float health)
{
	if (!registry.players.has(player_entity)) {
		return;
	}
	
	Player& player = registry.players.get(player_entity);
	player.health = (health < 0.0f) ? 0.0f : ((health > player.max_health) ? player.max_health : health);
}

void HealthSystem::set_max_health(Entity player_entity, float max_health)
{
	if (!registry.players.has(player_entity)) {
		return;
	}
	
	Player& player = registry.players.get(player_entity);
	player.max_health = std::max(1.0f, max_health);
	
	player.health = std::min(player.health, player.max_health);
}

float HealthSystem::get_health(Entity player_entity) const
{
	if (!registry.players.has(player_entity)) {
		return 0.0f;
	}
	
	const Player& player = registry.players.get(player_entity);
	return player.health;
}

float HealthSystem::get_max_health(Entity player_entity) const
{
	if (!registry.players.has(player_entity)) {
		return 0.0f;
	}
	
	const Player& player = registry.players.get(player_entity);
	return player.max_health;
}

float HealthSystem::get_health_percent(Entity player_entity) const
{
	if (!registry.players.has(player_entity)) {
		return 0.0f;
	}
	
	const Player& player = registry.players.get(player_entity);
	
	if (player.max_health <= 0) {
		return 0.0f;
	}
	
	return (float)player.health / (float)player.max_health * 100.0f;
}

bool HealthSystem::is_dead(Entity player_entity) const
{
	if (!registry.players.has(player_entity)) {
		return true;
	}
	
	const Player& player = registry.players.get(player_entity);
	return player.health <= 0.0f;
}

bool HealthSystem::is_full_health(Entity player_entity) const
{
	if (!registry.players.has(player_entity)) {
		return false;
	}
	
	const Player& player = registry.players.get(player_entity);
	return player.health >= player.max_health;
}

Entity HealthSystem::get_player_entity() const
{
	if (registry.players.entities.size() > 0) {
		return registry.players.entities[0];
	}
	return Entity();
}

bool HealthSystem::has_player() const
{
	return registry.players.entities.size() > 0;
}

void HealthSystem::update(float elapsed_ms)
{
	if (!has_player()) {
		return;
	}
	
	Entity player_entity = get_player_entity();
	
	// Don't heal if player is dead or at full health
	if (is_dead(player_entity) || is_full_health(player_entity)) {
		if (is_full_health(player_entity)) {
			time_since_last_damage_ms = 0.0f;
		}
		return;
	}
	
	time_since_last_damage_ms += elapsed_ms;
	
	// Check if enough time has passed since last damage
	if (time_since_last_damage_ms >= heal_delay_ms) {
		float heal_amount = heal_rate * (elapsed_ms / 1000.0f); // Convert ms to seconds
		
		heal(player_entity, heal_amount);
	}
}

void HealthSystem::set_heal_delay_ms(float delay_ms)
{
	heal_delay_ms = std::max(0.0f, delay_ms);
}

float HealthSystem::get_heal_delay_ms() const
{
	return heal_delay_ms;
}

void HealthSystem::set_heal_rate(float health_per_second)
{
	heal_rate = std::max(0.0f, health_per_second);
}

float HealthSystem::get_heal_rate() const
{
	return heal_rate;
}

void HealthSystem::reset_healing_timer()
{
	time_since_last_damage_ms = 0.0f;
}

