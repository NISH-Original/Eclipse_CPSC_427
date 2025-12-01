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
	
}


