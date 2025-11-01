#include "steering_system.hpp"

#include "tiny_ecs_registry.hpp"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

static inline float normalize_angle(float angle) {
    angle = std::fmod(angle, 2.0f * M_PI);
    if (angle < 0.0f) {
        angle += 2.0f * M_PI;
    }
    return angle;
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
    add_steering();
    update_motion(elapsed_ms);
}