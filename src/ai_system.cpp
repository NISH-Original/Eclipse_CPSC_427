// internal
#include "ai_system.hpp"
#include "world_init.hpp"
#include "audio_system.hpp"
#include <iostream>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923  // π/2
#endif

void AISystem::init(RenderSystem* renderer, AudioSystem* audio) {
	this->renderer = renderer;
	this->audio_system = audio;
}

void AISystem::step(float elapsed_ms)
{
	float step_seconds = elapsed_ms / 1000.f;
	enemyStep(step_seconds);
	spriteStep(step_seconds); // Should be at the very end to overwrite motion
	stationaryEnemyStep(step_seconds);
	dropStep(step_seconds);
	trailStep(step_seconds);
}

void AISystem::enemyStep(float step_seconds)
{
	auto& enemy_registry = registry.enemies;
	
	for(uint i = 0; i< enemy_registry.size(); i++) {
		Entity entity = enemy_registry.entities[i];
		Enemy& enemy = registry.enemies.get(entity);
		Motion& motion = registry.motions.get(entity);
		
		// Update healthbar visibility timer (decrease over time)
		if (enemy.healthbar_visibility_timer > 0.0f) {
			enemy.healthbar_visibility_timer -= step_seconds;
			if (enemy.healthbar_visibility_timer < 0.0f) {
				enemy.healthbar_visibility_timer = 0.0f;
			}
		}
		
		if (enemy.is_hurt && !enemy.is_dead) {
	    enemy.hurt_timer += step_seconds;

			if (enemy.hurt_timer > 0.2f) {
					enemy.is_hurt = false;
					enemy.hurt_timer = 0.f;
			}
			
			if(enemy.hurt_animation == NULL) {
			} else {
				enemy.hurt_animation(entity, step_seconds);
			}
		}

		if (enemy.is_dead) {
			if (!enemy.death_handled) {
				enemy.death_handled = true;

				if (on_enemy_killed) {
						on_enemy_killed();
				}

				if (registry.collisionCircles.has(entity)) {
					registry.collisionCircles.remove(entity);
				}

				if (registry.colliders.has(entity)) {
					registry.colliders.remove(entity);
				}
			}

			if(enemy.death_animation == NULL) {
				motion.angle += 3 * M_PI * step_seconds;
				motion.velocity = {0.0f, 0.0f};
				motion.scale -= glm::vec2(30.0f) * step_seconds;

				if (motion.scale.x < 0.f || motion.scale.y < 0.f) {
					registry.remove_all_components_of(entity);
				}
			} else {
				enemy.death_animation(entity, step_seconds);
			}

		} else {
			//glm::vec2 diff = player_motion.position - motion.position;
			//motion.angle = atan2(diff.y, diff.x);
			//motion.velocity = glm::normalize(diff) * 50.f;

		}
	}
}

// Functiton for stationaryEnemies (evil plants)
// TODO: Refactor enemy logic into proper ECS components and systems
void AISystem::stationaryEnemyStep(float step_seconds)
{
	const float detection_radius = 400.f;
	const float attack_radius = 200.f;
	const int spriteFaceRight = 3;
	const int spriteFaceLeft = 2;
	const int spriteFaceDown = 0;
	const int spriteFaceUp = 1;

	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& stationary_enemy_registry = registry.stationaryEnemies;
	
	for(uint i = 0; i< stationary_enemy_registry.size(); i++) {
		Entity entity = stationary_enemy_registry.entities[i];
		Enemy& enemy = registry.enemies.get(entity);
		StationaryEnemy& plant = registry.stationaryEnemies.get(entity);
		RenderRequest& render = registry.renderRequests.get(entity);

		Motion& motion = registry.motions.get(entity);
		motion.position = plant.position;
		motion.velocity = {0.0f, 0.0f};

		if (enemy.is_dead) continue;
		if (enemy.is_hurt) continue;
		if (registry.boss_parts.has(entity)) {
			continue;
		}

		Sprite& sprite = registry.sprites.get(entity);
		sprite.should_flip = false;

		vec2 diff = player_motion.position - motion.position;
		float dist = sqrt(diff.x * diff.x + diff.y * diff.y);
		bool player_in_detect = dist < detection_radius;
		bool player_in_attack = dist < attack_radius;

		switch (plant.state) {
			case StationaryEnemyState::EP_IDLE:
				if (player_in_detect)
					plant.state = StationaryEnemyState::EP_DETECT_PLAYER;
				break;

			case StationaryEnemyState::EP_DETECT_PLAYER:
				if (fabs(diff.x) > fabs(diff.y))
					plant.facing = (diff.x > 0) ? StationaryEnemyFacing::EP_FACING_RIGHT : StationaryEnemyFacing::EP_FACING_LEFT;
				else
					plant.facing = (diff.y > 0) ? StationaryEnemyFacing::EP_FACING_DOWN : StationaryEnemyFacing::EP_FACING_UP;

				switch (plant.facing) {
					case StationaryEnemyFacing::EP_FACING_RIGHT: 
						sprite.curr_row = spriteFaceRight;
						break;
					case StationaryEnemyFacing::EP_FACING_LEFT:
						sprite.curr_row = spriteFaceLeft;
						break;
					case StationaryEnemyFacing::EP_FACING_DOWN:
						sprite.curr_row = spriteFaceDown;
						break;
					case StationaryEnemyFacing::EP_FACING_UP:
						sprite.curr_row = spriteFaceUp;
						break;
				}

				if (player_in_attack) {
					plant.state = StationaryEnemyState::EP_ATTACK_PLAYER;
					render.used_texture = static_cast<TEXTURE_ASSET_ID>(
						static_cast<int>(render.used_texture) + 1
					);
					sprite.total_frame = 7;
					sprite.curr_frame = 0;
					sprite.step_seconds_acc = 0.f;
				} else {
					plant.state = StationaryEnemyState::EP_IDLE;
				}
				break;

			case StationaryEnemyState::EP_ATTACK_PLAYER:
				if (sprite.step_seconds_acc >= sprite.total_frame - 1) {
					vec2 dir = { diff.x / dist, diff.y / dist };
					float bullet_velocity = 400.f; 
					float plant_radius = motion.scale.x / 2;

					vec2 bullet_pos = {
						motion.position.x + plant_radius * dir.x,
						motion.position.y + plant_radius * dir.y
					};

					vec2 bullet_vel = {
						bullet_velocity * dir.x,
						bullet_velocity * dir.y
					};

					Entity entity = createBullet(renderer, bullet_pos, bullet_vel, enemy.damage);
					registry.deadlies.emplace(entity);

					render.used_texture = static_cast<TEXTURE_ASSET_ID>(
						static_cast<int>(render.used_texture) - 1
					);
					sprite.step_seconds_acc = 0.f;
					sprite.total_frame = 4;
					sprite.curr_frame = 0;
					
					plant.attack_cooldown = 2.f;
					plant.state = StationaryEnemyState::EP_COOLDOWN;
				}
				break;

			case StationaryEnemyState::EP_COOLDOWN:
				if (fabs(diff.x) > fabs(diff.y))
					plant.facing = (diff.x > 0) ? StationaryEnemyFacing::EP_FACING_RIGHT : StationaryEnemyFacing::EP_FACING_LEFT;
				else
					plant.facing = (diff.y > 0) ? StationaryEnemyFacing::EP_FACING_DOWN : StationaryEnemyFacing::EP_FACING_UP;

				switch (plant.facing) {
					case StationaryEnemyFacing::EP_FACING_RIGHT: 
						sprite.curr_row = spriteFaceRight;
						break;
					case StationaryEnemyFacing::EP_FACING_LEFT:
						sprite.curr_row = spriteFaceLeft;
						break;
					case StationaryEnemyFacing::EP_FACING_DOWN:
						sprite.curr_row = spriteFaceDown;
						break;
					case StationaryEnemyFacing::EP_FACING_UP:
						sprite.curr_row = spriteFaceUp;
						break;
				}

				plant.attack_cooldown -= step_seconds;
				if (plant.attack_cooldown <= 0.f) {
					plant.attack_cooldown = 0.f;
					plant.state = player_in_detect ? StationaryEnemyState::EP_DETECT_PLAYER
					                               : StationaryEnemyState::EP_IDLE;
				}
				break;
		}
	}
}

void AISystem::spriteStep(float step_seconds)
{
	auto& sprite_registry = registry.sprites;
	
	for(uint i = 0; i< sprite_registry.size(); i++) {
		Entity entity = sprite_registry.entities[i];
		Sprite& sprite = registry.sprites.get(entity);
		if (!sprite.animation_enabled) continue;

		sprite.step_seconds_acc += step_seconds * sprite.animation_speed;
		sprite.curr_frame = (int)std::floor(sprite.step_seconds_acc) % sprite.total_frame;

		// Disable rotation for entity with sprites (but not for players)
		if (registry.motions.has(entity) && !registry.players.has(entity) && !registry.feet.has(entity)) {
			Motion& motion = registry.motions.get(entity);
			if (motion.angle > M_PI_2 || motion.angle < -M_PI_2) {
				sprite.should_flip = true;
			} else {
				sprite.should_flip = false;
			}
			// motion.angle = 0;
		}
	}
}

void AISystem::dropStep(float step_seconds) {
  if (registry.players.size() == 0) return;

  Entity player = registry.players.entities[0];
	Player& p = registry.players.get(player);
  Motion& pm = registry.motions.get(player);
  vec2 player_pos = pm.position;

  auto& drops = registry.drops;

  const float pickup_radius = 130.f;
  const float initial_repel_speed = 220.f;
  const float repel_damping = 0.88f;
  const float base_accel = 6000.f;
  const float max_speed = 480.f;

  for (uint i = 0; i < drops.size(); i++) {
    Entity d = drops.entities[i];
    Drop& drop = drops.components[i];
    Motion& dm = registry.motions.get(d);

    vec2 diff = player_pos - dm.position;
    float dist = sqrt(diff.x * diff.x + diff.y * diff.y);

    if (!drop.is_magnetized && dist < pickup_radius) {
      drop.is_magnetized = true;
      drop.magnet_timer = 0.12f;

      vec2 dir = diff / (dist + 0.001f);

      float rand_ang = ((rand() % 50) - 25) * (M_PI / 180.f);
      float ca = cos(rand_ang), sa = sin(rand_ang);

      vec2 rotated = {
        dir.x * ca - dir.y * sa,
        dir.x * sa + dir.y * ca
      };

      dm.velocity = -rotated * initial_repel_speed;
      continue;
    }

    if (drop.is_magnetized && drop.magnet_timer > 0.f) {
      drop.magnet_timer -= step_seconds;
      dm.velocity *= repel_damping;

      if (length(dm.velocity) < 5.f)
        dm.velocity = {0.f, 0.f};

      continue;
    }

    if (drop.is_magnetized && drop.magnet_timer <= 0.f) {
      vec2 dir = diff / (dist + 0.001f);

      float accel_factor = glm::clamp(1.f - (dist / 250.f), 0.1f, 1.f);
      float accel = base_accel * accel_factor;

      dm.velocity += dir * accel * step_seconds;
      dm.velocity *= 0.92f;

      float speed = length(dm.velocity);
      if (speed > max_speed)
        dm.velocity = (dm.velocity / speed) * max_speed;

			drop.trail_accum += step_seconds;

			if (drop.trail_accum >= 0.04f) {
				drop.trail_accum = 0.f;

				if (registry.sprites.has(d) && registry.renderRequests.has(d)) {
					Sprite& src_sprite = registry.sprites.get(d);
					TEXTURE_ASSET_ID tex = registry.renderRequests.get(d).used_texture;
					Entity entitiy = create_drop_trail(dm, src_sprite);
					Trail& trail = registry.trails.get(entitiy);
					trail.is_red = tex == TEXTURE_ASSET_ID::FIRST_AID;
				}
			}

      if (dist < 20.f) {
  			TEXTURE_ASSET_ID tex = registry.renderRequests.get(d).used_texture;
				
				if(tex == TEXTURE_ASSET_ID::XYLARITE) {
					p.currency += 10;
					// Play xylarite collect sound
					if (audio_system) {
						audio_system->play("xylarite_collect");
					}
				} else {
					p.health += 30;
					p.health = min(p.health, p.max_health);
				}
        registry.remove_all_components_of(d);
        continue;

      }
    }
  }
}

void AISystem::trailStep(float step_seconds) {
  auto& trails = registry.trails;

  for (uint i = 0; i < trails.size(); ) {
    Entity e = trails.entities[i];
    Trail& t = trails.components[i];

    t.life -= step_seconds;
    if (t.life <= 0.f) {
      registry.remove_all_components_of(e);
      continue;
    }

    if (registry.motions.has(e)) {
      Motion& m = registry.motions.get(e);

      float shrink = 1.f - step_seconds * 4.f;
      if (shrink < 0.7f) shrink = 0.7f;

      m.scale.x *= shrink;
      m.scale.y *= shrink;
    }

    t.alpha *= (1.f - step_seconds * 5.f);
    if (t.alpha < 0.01f) {
      registry.remove_all_components_of(e);
      continue;
    }

    ++i;
  }
}
