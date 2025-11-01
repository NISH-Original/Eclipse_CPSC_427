#include "steering_system.hpp"

#include "tiny_ecs_registry.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

// TODO copied from pathfinding, make it common
constexpr glm::ivec2 DIRECTIONS[] = {
    { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
    { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
};

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
    return DIRECTIONS[idx];
}

static void add_avoid_force() {
    auto& motions_registry = registry.motions;
    auto& dirs_registry = registry.enemy_dirs;
    for (const auto& e : registry.enemies.entities) {
        const Motion& me = motions_registry.get(e);
        AccumulatedForce& af = dirs_registry.get(e);
        
        glm::vec2 avoid{ 0.0f, 0.0f };
        for (int i = 1; i <= 3; i++) {
            if (i == 3) { // TODO check for blocked
                glm::ivec2 obstacle_dir = snap_octagonal(me.angle);
                glm::vec2 avoid_cw{ obstacle_dir.y, -obstacle_dir.x };
                glm::vec2 avoid_ccw{ -obstacle_dir.y, obstacle_dir.x };

                // Pick direction closest to current velocity
                glm::vec2 avoid_dir{ 0, 0 };
                if (glm::dot(me.velocity, avoid_cw) > glm::dot(me.velocity, avoid_ccw)) {
                    avoid_dir = glm::normalize(avoid_cw);
                } else {
                    avoid_dir = glm::normalize(avoid_ccw);
                }

                // TODO this is hardcoded avoid force magnitude and safe distance
                float force_ratio = (4 - i) / 4.0f;
                float magnitude = 2.0f * force_ratio;
                avoid = avoid_dir * magnitude;
                break;
            }
        }
        
        af.v += avoid;
    }
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

static void update_motion(float elapsed_ms) {
    const auto& steering_registry = registry.enemy_steerings;
    for (int i = 0; i < steering_registry.components.size(); i++) {
        Motion& motion_comp = registry.motions.get(steering_registry.entities[i]);
        const Steering& steering_comp = steering_registry.components[i];

        float angle_diff = steering_comp.target_angle - motion_comp.angle;
        float shortest_diff = glm::atan(glm::sin(angle_diff), glm::cos(angle_diff));
        if (glm::abs(shortest_diff) < ROTATE_EPSILON) {
            // TODO remove component
        }

        float max_rad = steering_comp.rad_ms * elapsed_ms;
        float frame_rad = glm::min(glm::abs(shortest_diff), max_rad);

        motion_comp.angle = normalize_angle(motion_comp.angle + frame_rad * glm::sign(shortest_diff));
        motion_comp.velocity = glm::vec2(cos(motion_comp.angle), sin(motion_comp.angle)) * 50.f;
    }
}

void SteeringSystem::step(float elapsed_ms) {
    // TODO gather all forces
    add_avoid_force();
    add_steering();
    update_motion(elapsed_ms);
}