#include "steering_system.hpp"

#include "tiny_ecs_registry.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

// TODO copied from pathfinding, make it common
constexpr glm::ivec2 DIRECTIONS[] = {
    { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
    { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
};

static inline glm::ivec2 get_cell_coordinate(glm::vec2 world_pos) {
    return glm::floor(world_pos / static_cast<float>(CHUNK_CELL_SIZE));
}

static inline CHUNK_CELL_STATE get_cell_state(const glm::ivec2& cell_pos) {
    if (registry.chunks.has(cell_pos.x / CHUNK_CELLS_PER_ROW, cell_pos.y / CHUNK_CELLS_PER_ROW)) {
        const Chunk& chunk = registry.chunks.get(cell_pos.x / CHUNK_CELLS_PER_ROW, cell_pos.y / CHUNK_CELLS_PER_ROW);
        return chunk.cell_states[cell_pos.x % CHUNK_CELLS_PER_ROW][cell_pos.y % CHUNK_CELLS_PER_ROW];
    } else {
        // chunk is not loaded, treat as having no obstacles
        return CHUNK_CELL_STATE::EMPTY;
    }
}

static inline float normalize_angle(float angle) {
    angle = std::fmod(angle, 2.0f * M_PI);
    if (angle < 0.0f) {
        angle += 2.0f * M_PI;
    }
    return angle;
}

static inline glm::ivec2 snap_octagonal(float angle) {
    float angle_shift = normalize_angle(angle + M_PI / 8.0f);
    int idx = static_cast<int>(angle_shift / (M_PI / 4));
    // NOTE: temporary patch for #47
    if (idx < 0 || idx >= 8)
        idx = 0;
    return DIRECTIONS[idx];
}

static void add_avoid_force() {
    auto& motions_registry = registry.motions;
    auto& dirs_registry = registry.enemy_dirs;
    for (const auto& e : registry.enemies.entities) {
        const Motion& me = motions_registry.get(e);
        AccumulatedForce& af = dirs_registry.get(e);
        
        glm::vec2 avoid{ 0.0f, 0.0f };
        glm::ivec2 obstacle_dir = snap_octagonal(glm::atan(af.v.y, af.v.x));
        glm::vec2 avoid_cw{ obstacle_dir.y, -obstacle_dir.x };
        glm::vec2 avoid_ccw{ -obstacle_dir.y, obstacle_dir.x };
        for (int i = 1; i <= 5; i++) {
            glm::ivec2 check_pos = get_cell_coordinate(me.position) + obstacle_dir * i;

            if (get_cell_state(check_pos) == CHUNK_CELL_STATE::OBSTACLE) {
                // Pick direction closest to current velocity
                glm::vec2 avoid_dir{ 0, 0 };
                if (glm::dot(me.velocity, avoid_cw) > glm::dot(me.velocity, avoid_ccw)) {
                    avoid_dir = glm::normalize(avoid_cw);
                } else {
                    avoid_dir = glm::normalize(avoid_ccw);
                }

                // TODO this is hardcoded avoid force magnitude and safe distance
                float force_ratio = (6 - i) / 6.0f;
                float magnitude = 5.0f * force_ratio;
                avoid = avoid_dir * magnitude;
                break;
            }
        }
        
        af.v += avoid;
    }
}

void add_flocking_force() {
    //
}

static void add_steering() {
    const auto& dirs_registry = registry.enemy_dirs;
    auto& steering_registry = registry.enemy_steerings;
    for (int i = 0; i < dirs_registry.components.size(); i++) {
        const glm::vec2& afv = glm::normalize(dirs_registry.components[i].v);
        const Entity& e = dirs_registry.entities[i];
        if (steering_registry.has(e)) {
            steering_registry.get(e).target_angle = glm::atan(afv.y, afv.x);
        } else {
            registry.enemy_steerings.insert(e, { glm::atan(afv.y, afv.x) });
        }
    }
}

// Returns true if the enemy position is inside the flashlight beam
// Used for flashlight upgrades that slow and damage enemies
static bool is_in_flashlight_beam(const glm::vec2& enemy_pos) {
    // Loop through all lights
    for (int i = 0; i < registry.lights.components.size(); i++) {
        const Light& light = registry.lights.components[i];
        // Only care about cone lights that are on, cone angle less than 2 means its a flashlight
        if (light.cone_angle < 2.0f && light.is_enabled) {
            Entity light_entity = registry.lights.entities[i];
            if (!registry.motions.has(light_entity)) continue;

            const Motion& light_motion = registry.motions.get(light_entity);
            // Vector from light to enemy
            glm::vec2 to_enemy = enemy_pos - light_motion.position;
            float dist = glm::length(to_enemy);

            // Skip if enemy is too far
            if (dist > light.range) continue;

            // Figure out what angle the enemy is at relative to where the light is pointing
            float angle_to_enemy = glm::atan(to_enemy.y, to_enemy.x);
            float angle_diff = angle_to_enemy - light_motion.angle;
            // Normalize angle difference to be between negative pi and pi
            angle_diff = glm::atan(glm::sin(angle_diff), glm::cos(angle_diff));

            // Enemy is within the cone angle so they are in the beam
            if (glm::abs(angle_diff) < light.cone_angle) {
                return true;
            }
        }
    }
    return false;
}

static void update_motion(float elapsed_ms) {
    const auto& steering_registry = registry.enemy_steerings;

    Entity player = registry.players.entities[0];
    Motion& player_motion = registry.motions.get(player);

    constexpr float BASE_SPEED = 140.f;
    constexpr float FLASHLIGHT_SLOW_SPEED = 50.f;
    constexpr float LUNGE_SPEED = 500.f;
    constexpr float LUNGE_RADIUS = 150.f;

    float step_seconds = elapsed_ms / 1000.f;

    int flashlight_slow_level = 0;
    int flashlight_damage_level = 0;
    if (registry.playerUpgrades.has(player)) {
        PlayerUpgrades& upgrades = registry.playerUpgrades.get(player);
        flashlight_slow_level = upgrades.flashlight_slow_level;
        flashlight_damage_level = upgrades.flashlight_damage_level;
    }

    // Calculate how slow enemies move when in flashlight based on upgrade level
    float slow_speed = FLASHLIGHT_SLOW_SPEED - (flashlight_slow_level * PlayerUpgrades::FLASHLIGHT_SLOW_PER_LEVEL);
    if (slow_speed < 10.f) slow_speed = 10.f;

    // Calculate flashlight damage per second based on upgrade level
    float damage_per_second = flashlight_damage_level * PlayerUpgrades::FLASHLIGHT_DAMAGE_PER_LEVEL;

    for (int i = 0; i < steering_registry.components.size(); i++) {
        Entity e = steering_registry.entities[i];
        if (registry.arrows.has(e)) {
            continue;
        }

        if(registry.enemies.has(e)) {
            Enemy& enemy = registry.enemies.get(e);
            if(enemy.is_hurt) continue;
        }

        Motion& motion_comp = registry.motions.get(e);
        const Steering& steering_comp = steering_registry.components[i];

        if (!registry.enemy_lunges.has(e)) {
            registry.enemy_lunges.emplace(e);
        }
        EnemyLunge& lunge = registry.enemy_lunges.get(e);

        // Tick down lunge cooldown
        if (lunge.lunge_cooldown > 0.f) {
            lunge.lunge_cooldown -= step_seconds;
        }

        // Get direction and distance to player
        glm::vec2 diff = player_motion.position - motion_comp.position;
        float dist = glm::length(diff);

        bool in_flashlight = is_in_flashlight_beam(motion_comp.position);

        // Apply flashlight burn damage if enemy is in flashlight and player has the upgrade
        if (in_flashlight && damage_per_second > 0.f && registry.enemies.has(e)) {
            Enemy& enemy = registry.enemies.get(e);
            if (!enemy.is_dead) {
                float damage_this_frame = damage_per_second * step_seconds;
                enemy.health -= (int)ceil(damage_this_frame);
                enemy.healthbar_visibility_timer = 3.0f;
                // Kill enemy if health drops to zero and remove collision
                if (enemy.health <= 0) {
                    enemy.is_dead = true;
                    registry.collisionCircles.remove(e);
                }
            }
        }

        // Handle lunge attack movement
        if (lunge.is_lunging) {
            lunge.lunge_timer -= step_seconds;
            if (lunge.lunge_timer <= 0.f) {
                lunge.is_lunging = false;
                lunge.lunge_cooldown = EnemyLunge::LUNGE_COOLDOWN;
            }
            motion_comp.velocity = lunge.lunge_direction * LUNGE_SPEED;
        } else {
            float angle_diff = steering_comp.target_angle - motion_comp.angle;
            float shortest_diff = glm::atan(glm::sin(angle_diff), glm::cos(angle_diff));

            float max_rad = steering_comp.rad_ms * elapsed_ms;
            float frame_rad = glm::min(glm::abs(shortest_diff), max_rad);

            motion_comp.angle = normalize_angle(motion_comp.angle + frame_rad * glm::sign(shortest_diff));

            // Slow enemies down if they are in flashlight
            float current_speed = in_flashlight ? slow_speed : BASE_SPEED;

            // Start lunge attack if close enough to player and not in flashlight
            if (dist < LUNGE_RADIUS && lunge.lunge_cooldown <= 0.f && !in_flashlight) {
                lunge.is_lunging = true;
                lunge.lunge_timer = EnemyLunge::LUNGE_DURATION;
                lunge.lunge_direction = glm::normalize(diff);
                motion_comp.velocity = lunge.lunge_direction * LUNGE_SPEED;
            } else {
                motion_comp.velocity = glm::vec2(cos(motion_comp.angle), sin(motion_comp.angle)) * current_speed;
            }
        }

        // Squish animation for moving enemies
        if (registry.movementAnimations.has(e)) {
            MovementAnimation& anim = registry.movementAnimations.get(e);
            anim.animation_timer += step_seconds;

            float speed = glm::length(motion_comp.velocity);
            if (speed > 10.f) {
                float squish_phase = anim.animation_timer * anim.squish_frequency;
                float squish = glm::sin(squish_phase) * anim.squish_amount;

                motion_comp.scale.x = anim.base_scale.x * (1.0f + squish);
                motion_comp.scale.y = anim.base_scale.y * (1.0f - squish);
            } else {
                motion_comp.scale = anim.base_scale;
            }
        }

        // Swap enemy sprite based on health percentage to show damage
        if (registry.enemies.has(e) && registry.renderRequests.has(e)) {
            RenderRequest& render = registry.renderRequests.get(e);
            bool is_enemy1_type = render.used_texture == TEXTURE_ASSET_ID::ENEMY1 ||
                                  render.used_texture == TEXTURE_ASSET_ID::ENEMY1_DMG1 ||
                                  render.used_texture == TEXTURE_ASSET_ID::ENEMY1_DMG2 ||
                                  render.used_texture == TEXTURE_ASSET_ID::ENEMY1_DMG3;

            if (is_enemy1_type) {
                Enemy& enemy = registry.enemies.get(e);
                float health_pct = (float)enemy.health / (float)enemy.max_health;

                // Different damage sprites for different health thresholds
                if (health_pct < 0.3f) {
                    render.used_texture = TEXTURE_ASSET_ID::ENEMY1_DMG3;
                } else if (health_pct < 0.6f) {
                    render.used_texture = TEXTURE_ASSET_ID::ENEMY1_DMG2;
                } else if (health_pct < 1.0f) {
                    render.used_texture = TEXTURE_ASSET_ID::ENEMY1_DMG1;
                } else {
                    render.used_texture = TEXTURE_ASSET_ID::ENEMY1;
                }
            }
        }
    }
}

void SteeringSystem::step(float elapsed_ms) {
    add_avoid_force();
    add_flocking_force();
    add_steering();
    update_motion(elapsed_ms);
}