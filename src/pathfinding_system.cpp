#include "pathfinding_system.hpp"

#include "tiny_ecs_registry.hpp"

#include <queue>
#include <utility>

static inline bool is_in_bounds(const CellCoordinate& pos, int size) {
    return pos.x >= 0 && pos.y >= 0 && pos.x < size && pos.y < size;
}

// Doesn't actually do radius, it checks a square region.
// Parameter `r` doesn't count for the center cell itself.
static inline bool is_coordinate_in(const CellCoordinate& coordinate, const CellCoordinate& center, int r) {
    // Both inclusive
    CellCoordinate lower = center - r;
    CellCoordinate upper = center + r;
    return coordinate.x >= lower.x && coordinate.y >= lower.y && coordinate.x < upper.x && coordinate.y < upper.y;
}

static inline int move_cost(const CellCoordinate& dir) {
    return (dir.x && dir.y) ? DIAGONAL_COST : CARDINAL_COST;
}

// Gets the global cell coordinate, not the local ones.
static inline CellCoordinate get_cell_coordinate(glm::vec2 world_pos) {
    return glm::floor(world_pos / static_cast<float>(CHUNK_CELL_SIZE));
}

static inline CHUNK_CELL_STATE get_cell_state(const CellCoordinate& cell_pos) {
    if (registry.chunks.has(cell_pos.x / CHUNK_CELLS_PER_ROW, cell_pos.y / CHUNK_CELLS_PER_ROW)) {
        const Chunk& chunk = registry.chunks.get(cell_pos.x / CHUNK_CELLS_PER_ROW, cell_pos.y / CHUNK_CELLS_PER_ROW);
        return chunk.cell_states[cell_pos.x % CHUNK_CELLS_PER_ROW][cell_pos.y % CHUNK_CELLS_PER_ROW];
    } else {
        // chunk is not loaded, treat as having no obstacles
        return CHUNK_CELL_STATE::EMPTY;
    }
}

void PathfindingSystem::build_flow_field() {
    const Entity& player = registry.players.entities[0];
    const Motion& mp = registry.motions.get(player);
    CellCoordinate top_left = get_cell_coordinate(mp.position) - FIELD_RADIUS;

    // Reset flow field with obstacle info from grid
    for (int y = 0; y < FIELD_SIZE; y++) {
        for (int x = 0; x < FIELD_SIZE; x++) {
            flow_field[y][x] = {};
            flow_field[y][x].walkable = (
                get_cell_state(top_left + CellCoordinate{ x, y }) != CHUNK_CELL_STATE::OBSTACLE
            );
        }
    }

    CellCoordinate goal{ FIELD_RADIUS, FIELD_RADIUS };
    if (!is_in_bounds(goal, FIELD_SIZE)) return;

    // Order by min cost
    auto cmp = [](const std::pair<float, CellCoordinate>& a, const std::pair<float, CellCoordinate>& b) {
        return a.first > b.first;
    };
    std::priority_queue<std::pair<float, CellCoordinate>, std::vector<std::pair<float, CellCoordinate>>, decltype(cmp)> pq(cmp);

    flow_field[goal.y][goal.x].cost = 0;
    pq.push({ 0, goal });

    while (!pq.empty()) {
        const auto& curr = pq.top();
        float curr_cost = curr.first;
        CellCoordinate curr_pos = curr.second;
        pq.pop();

        // Not worth exploring, calculated cost greater than current min
        if (curr_cost > flow_field[curr_pos.y][curr_pos.x].cost) continue;

        // Explore all neighbours
        for (const auto& dir : DIRECTIONS) {
            CellCoordinate next_pos = curr_pos + dir;
            if (!is_in_bounds(next_pos, FIELD_SIZE)) continue;

            auto& neighbour = flow_field[next_pos.y][next_pos.x];
            if (!neighbour.walkable) {
                continue; }

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
    CellCoordinate player_cell = get_cell_coordinate(motions_registry.get(player).position);
    for (const auto& e : registry.enemies.entities) {
        if (!dirs_registry.has(e)) {
            dirs_registry.emplace(e);
        }

        AccumulatedForce& af = dirs_registry.get(e);
        af.v = { 0, 0 };

        CellCoordinate enemy_cell = get_cell_coordinate(motions_registry.get(e).position);
        if (is_coordinate_in(enemy_cell, player_cell, FIELD_RADIUS)) {
        //if (false) {
            CellCoordinate field_pos = enemy_cell - (player_cell - FIELD_RADIUS);
            af.v += glm::normalize(glm::vec2(flow_field[field_pos.y % CHUNK_CELLS_PER_ROW][field_pos.x % CHUNK_CELLS_PER_ROW].dir));
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