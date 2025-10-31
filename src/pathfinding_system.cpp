#include "pathfinding_system.hpp"

#include <iostream>
#include <queue>

static inline bool is_in_bounds(const PathVector& pos, int size) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < size && pos.y < size;
}

static inline int move_cost(const PathVector& dir) {
    return (dir.x && dir.y) ? DIAGONAL_COST : CARDINAL_COST;
}

void PathfindingSystem::build_flow_field() {
    //glm::vec2 origin = { player.x - FIELD_RADIUS, player.y - FIELD_RADIUS };
    //for (int y = 0; y < FIELD_SIZE; y++) {
    //    for (int x = 0; x < FIELD_SIZE; x++) {
    //        glm::vec2 world_pos = { origin.x + x, origin.y + y };
    //        flow_field[y][x].walkable = true; // get from world pos
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
        glm::ivec2 curr_pos = curr.second;
        pq.pop();

        // Not worth exploring, calculated cost greater than current min
        if (curr_cost > flow_field[curr_pos.y][curr_pos.x].cost) continue;

        // Explore all neighbours
        for (const auto& dir : DIRECTIONS) {
            glm::ivec2 next_pos = curr_pos + dir;
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

void PathfindingSystem::step(float elapsed_ms) {
    build_flow_field();
    // TODO conditional check for player movement
    // TODO add force to any enemies on flow field
}