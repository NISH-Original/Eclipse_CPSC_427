// internal
#include "ai_system.hpp"
#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>

void AISystem::step(float elapsed_ms)
{
	enemyStep(elapsed_ms);
}

void AISystem::enemyStep(float elapsed_ms)
{
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& enemy_registry = registry.enemies;
	
	for(uint i = 0; i< enemy_registry.size(); i++) {
		Entity entity = enemy_registry.entities[i];
		Motion& motion = registry.motions.get(entity);
		glm::vec2 diff = player_motion.position - motion.position; 
		motion.velocity = glm::normalize(diff) * 50.f;
	}
}

using grid_t = std::vector<std::vector<int>>;
constexpr int CARDINAL_COST = 10;
constexpr int DIAGONAL_COST = 14;

struct Point {
    int x, y;

    // for map
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct Node {
    Point pos;
    int g_cost;
    int h_cost;
    int f_cost;
    Point parent_pos;

    Node(Point p, int g, int h, Point parent)
        : pos(p), g_cost(g), h_cost(h), f_cost(g + h), parent_pos(parent) {}
};

// for pque
struct CompareNode {
    bool operator()(const Node& a, const Node& b) const {
        if (a.f_cost != b.f_cost)
            return a.f_cost > b.f_cost;
        else
            return a.g_cost < b.g_cost;
    }
};

static int heuristic(const Point& a, const Point& b) {
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    return 10 * max(dx, dy) + 4 * min(dx, dy); // 14 for diagonal, 10 for cardinal
}

static bool walkable(Point p, const grid_t& grid) {
    if (p.x < 0 || p.x >= grid.size() || p.y < 0 || p.y >= grid[0].size()) return false;
    return grid[p.x][p.y] == 0;
}

static std::vector<Point> reconstruct(const Point& goal, const std::map<Point, Node>& all_nodes) {
    std::vector<Point> path;
    Point current = goal;
    if (all_nodes.find(goal) == all_nodes.end()) return path;

    while (!(current.x == all_nodes.at(current).parent_pos.x && current.y == all_nodes.at(current).parent_pos.y)) {
        path.push_back(current);
        current = all_nodes.at(current).parent_pos;
        if (path.size() > all_nodes.size()) break;
    }
    path.push_back(current);
    std::reverse(path.begin(), path.end());
    return path;
}

static std::vector<Point> findPath(const Point& start, const Point& goal, const grid_t& grid) {
    if (!walkable(start, grid) || !walkable(goal, grid)) return {};
    else if (start == goal) return { start };

    std::priority_queue<Node, std::vector<Node>, CompareNode> open_set; // f-cost
    std::map<Point, int> g_costs; // g-cost
    std::map<Point, Node> all_nodes; // reconstruction

    Point dummy_parent = { -1, -1 };
    Node start_node(start, 0, heuristic(start, goal), dummy_parent);

    open_set.push(start_node);
    g_costs[start] = 0;
    all_nodes[start] = start_node;

    constexpr int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    constexpr int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    while (!open_set.empty()) {
        Node curr = open_set.top();
        open_set.pop();
        Point curr_pos = curr.pos;

        if (curr.g_cost > g_costs[curr_pos]) continue;
        if (curr_pos == goal) {
            all_nodes[curr_pos] = curr;
            return reconstruct(goal, all_nodes);
        }

        for (int i = 0; i < 8; ++i) {
            Point neighbor_pos = { curr_pos.x + dx[i], curr_pos.y + dy[i] };
            if (!walkable(neighbor_pos, grid)) continue;

            int move_cost = (abs(dx[i]) + abs(dy[i]) == 2) ? DIAGONAL_COST : CARDINAL_COST;
            int tentative_g_cost = curr.g_cost + move_cost;

            // if better
            if (g_costs.find(neighbor_pos) == g_costs.end() || tentative_g_cost < g_costs[neighbor_pos]) {
                g_costs[neighbor_pos] = tentative_g_cost;
                int h = heuristic(neighbor_pos, goal);
                Node neighbor_node(neighbor_pos, tentative_g_cost, h, curr_pos);
                all_nodes[neighbor_pos] = neighbor_node;
                open_set.push(neighbor_node);
            }
        }
    }

    return {};
}
