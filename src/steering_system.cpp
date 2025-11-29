#include "steering_system.hpp"

#include "tiny_ecs_registry.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/hash.hpp>

// TODO copied from pathfinding, make it common
constexpr glm::ivec2 DIRECTIONS[] = {
    { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 },
    { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 }
};

constexpr float SEPARATION_WEIGHT = 100.f;
constexpr float ALIGNMENT_WEIGHT = 50.f;
constexpr float COHESION_WEIGHT = 20.f;
constexpr float VELOCITY_FACTOR = 20.f;

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

static std::unordered_map<glm::ivec2, Entity> find_neighbours() {
    std::unordered_map<glm::ivec2, Entity> neighbour_map;

    for (const auto& e : registry.enemy_dirs.entities) {
        const auto& me = registry.motions.get(e);
        neighbour_map[get_cell_coordinate(me.position)] = e;
    }

    return neighbour_map;
}

static void add_flocking_force() {
    std::unordered_map<glm::ivec2, Entity> neighbour_map = find_neighbours();
    auto& motion_registry = registry.motions;
    auto& dirs_registry = registry.enemy_dirs;
    const auto& mp = motion_registry.get(registry.players.entities[0]);
    for (const auto& e : registry.enemy_dirs.entities) {
        auto& me = motion_registry.get(e);
        auto& af = dirs_registry.get(e);

        glm::vec2 separation{ 0, 0 };
        glm::vec2 alignment{ 0, 0 };
        glm::vec2 cohesion{ 0, 0 };

        int n_neighbours = 0;
        for (int i = -3; i <= 3; i++) {
            for (int j = -3; j <= 3; j++) {
                if (!i && !j) continue;

                glm::ivec2 curr_cell = get_cell_coordinate(me.position) + glm::ivec2{ i, j };
                if (neighbour_map.count(curr_cell)) {
                    const Entity& neighbour = neighbour_map[curr_cell];
                    auto& mn = motion_registry.get(neighbour);

                    // Separation force
                    glm::vec2 diff = me.position - mn.position;
                    diff = glm::normalize(diff) / glm::length(diff) * SEPARATION_WEIGHT;
                    separation += diff;

                    // Alignment force
                    alignment += mn.velocity;
                    
                    // Cohesion force
                    cohesion += mn.position;

                    n_neighbours++;
                }
            }
        }

        // Cancel cohesion at about 45 deg deviation
        if (glm::dot(me.velocity, mp.position - me.position) < 0.7) {
            alignment = { 0.f, 0.f };
            cohesion = { 0.f, 0.f };
        }
        
        if (n_neighbours)
            af.v = af.v + separation + alignment / (float) n_neighbours * ALIGNMENT_WEIGHT + cohesion / (float) n_neighbours * COHESION_WEIGHT;
    }
}

static void add_steering() {
    const auto& dirs_registry = registry.enemy_dirs;
    auto& steering_registry = registry.enemy_steerings;
    for (int i = 0; i < dirs_registry.components.size(); i++) {
        const glm::vec2& afv = dirs_registry.components[i].v;
        const Entity& e = dirs_registry.entities[i];
        if (steering_registry.has(e)) {
            steering_registry.get(e).target_angle = glm::atan(afv.y, afv.x);
            steering_registry.get(e).vel = glm::length(afv);
        } else {
            registry.enemy_steerings.insert(e, { glm::atan(afv.y, afv.x), 0.003, glm::length(afv) });
        }
    }
}

static void update_motion(float elapsed_ms) {
    const auto& steering_registry = registry.enemy_steerings;
    for (int i = 0; i < steering_registry.components.size(); i++) {
        Entity e = steering_registry.entities[i];
        // Skip arrows - they are static and should never have steering
        if (registry.arrows.has(e)) {
            continue;
        }

        if(registry.enemies.has(e)) {
            Enemy& enemy = registry.enemies.get(e);
            if(enemy.is_hurt) continue;
        }

        Motion& motion_comp = registry.motions.get(e);
        const Steering& steering_comp = steering_registry.components[i];

        float angle_diff = steering_comp.target_angle - motion_comp.angle;
        float shortest_diff = glm::atan(glm::sin(angle_diff), glm::cos(angle_diff));
        //if (glm::abs(shortest_diff) < ROTATE_EPSILON) {
        //    continue;
        //}

        float max_rad = steering_comp.rad_ms * elapsed_ms;
        float frame_rad = glm::min(glm::abs(shortest_diff), max_rad);

        motion_comp.angle = normalize_angle(motion_comp.angle + frame_rad * glm::sign(shortest_diff));
        motion_comp.velocity = glm::vec2(cos(motion_comp.angle), sin(motion_comp.angle)) * glm::clamp(steering_comp.vel, 0.f, 2000.f) / VELOCITY_FACTOR;
    }
}

void SteeringSystem::step(float elapsed_ms) {
    add_avoid_force();
    add_flocking_force();
    add_steering();
    update_motion(elapsed_ms);
}