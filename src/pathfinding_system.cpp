#include "pathfinding_system.hpp"

#include "tiny_ecs_registry.hpp"

#include <iostream>
#include <queue>
#include <utility>

// TODO copied from world system, need to move to common
const size_t CHUNK_CELLS_PER_ROW = (size_t)window_width_px / 20;
const size_t CHUNK_CELLS_PER_COLUMN = (size_t)window_height_px / 20;

static inline bool is_in_bounds(const PathVector& pos, int size) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < size && pos.y < size;
}

static inline int move_cost(const PathVector& dir) {
    return (dir.x && dir.y) ? DIAGONAL_COST : CARDINAL_COST;
}

void PathfindingSystem::build_flow_field() {
    //const Entity& player = registry.players.entities[0];
    //const Chunk& chunk = registry.chunks.get(0, 0);
    //const Motion& mp = registry.motions.get(player);
    //PathVector player_chunk = PathVector(glm::floor(mp.position / glm::vec2(CHUNK_CELLS_PER_ROW, CHUNK_CELLS_PER_COLUMN)));
    //for (int y = 0; y < FIELD_SIZE; y++) {
    //    for (int x = 0; x < FIELD_SIZE; x++) {
    //        PathVector world_pos = { player_chunk.x + x, player_chunk.y + y };
    //        flow_field[y][x].walkable = (chunk.cell_states[world_pos.y][world_pos.x] == CHUNK_CELL_STATE::WALKABLE);
    //    }
    //}

    PathVector goal{ FIELD_RADIUS, FIELD_RADIUS };
    if (!is_in_bounds(goal, FIELD_SIZE)) return;

    // Order by min cost
    auto cmp = [](const std::pair<float, PathVector>& a, const std::pair<float, PathVector>& b) {
        return a.first > b.first;
    };
    std::priority_queue<std::pair<float, PathVector>, std::vector<std::pair<float, PathVector>>, decltype(cmp)> pq(cmp);

    flow_field[goal.y][goal.x].cost = 0;
    pq.push({ 0, goal });

    while (!pq.empty()) {
        const auto& curr = pq.top();
        float curr_cost = curr.first;
        PathVector curr_pos = curr.second;
        pq.pop();

        // Not worth exploring, calculated cost greater than current min
        if (curr_cost > flow_field[curr_pos.y][curr_pos.x].cost) continue;

        // Explore all neighbours
        for (const auto& dir : DIRECTIONS) {
            PathVector next_pos = curr_pos + dir;
            if (!is_in_bounds(next_pos, FIELD_SIZE)) continue;

            auto& neighbour = flow_field[next_pos.y][next_pos.x];
            if (!neighbour.walkable) continue;

            int next_cost = curr_cost + move_cost(dir);
            if (next_cost < neighbour.cost) { // Shorter path found
                neighbour.cost = next_cost;
                neighbour.dir = { -dir.x, -dir.y };
                pq.push({ next_cost, next_pos });
            }
        }
    }
}

void PathfindingSystem::add_path_force() {
    const Entity& player = registry.players.entities[0];
    auto& motions_registry = registry.motions;
    auto& dirs_registry = registry.enemy_dirs;
    for (const auto& e : registry.enemies.entities) {
        if (!dirs_registry.has(e)) {
            dirs_registry.emplace(e);
        }

        AccumulatedForce& af = dirs_registry.get(e);
        af.v = { 0, 0 };
        if (false) { // TODO with in flow field
            af.v += glm::normalize(glm::vec2(flow_field[0][0].dir));
        } else {
            const Motion& mp = motions_registry.get(player);
            const Motion& me = motions_registry.get(e);
            af.v += glm::normalize(mp.position - me.position);
        }
    }
}

void PathfindingSystem::step(float elapsed_ms) {
    build_flow_field();
    add_path_force();
}