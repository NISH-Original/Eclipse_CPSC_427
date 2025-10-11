// internal
#include "ai_system.hpp"
#include <iostream>
#include <queue>
#include <cmath>
#include <algorithm>
#include <map>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923  // π/2
#endif

using grid_t = std::vector<std::vector<int>>;
constexpr int CARDINAL_COST = 10;
constexpr int DIAGONAL_COST = 14;

struct Point {
    int x, y;

    Point() = default;
    Point(int _x, int _y) : x(_x), y(_y) {}
    Point(const glm::vec2& v) : x(v.x), y(v.y) {}

    // for map
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct PathNode {
    Point pos;
    int g_cost;
    int h_cost;
    int f_cost;
    Point parent_pos;

    PathNode() = default;
    PathNode(Point p, int g, int h, Point parent)
        : pos(p), g_cost(g), h_cost(h), f_cost(g + h), parent_pos(parent) {
    }
};

// for pque
struct CompareNode {
    bool operator()(const PathNode& a, const PathNode& b) const {
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

static std::vector<Point> reconstruct(const Point& goal, const std::map<Point, PathNode>& all_nodes) {
    std::vector<Point> path;
    Point curr = goal;
    if (all_nodes.find(goal) == all_nodes.end()) return path;

    while (!(curr.x == all_nodes.at(curr).parent_pos.x && curr.y == all_nodes.at(curr).parent_pos.y)) {
        path.push_back(curr);
        curr = all_nodes.at(curr).parent_pos;
        if (curr.x == -1 || path.size() > all_nodes.size()) break;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

static std::vector<Point> findPath(const Point& start, const Point& goal, const grid_t& grid) {
    //if (!walkable(start, grid) || !walkable(goal, grid)) return {};
    //else if (start == goal) return { start };

    std::priority_queue<PathNode, std::vector<PathNode>, CompareNode> open_set; // f-cost
    std::map<Point, int> g_costs; // g-cost
    std::map<Point, PathNode> all_nodes; // reconstruction

    Point dummy_parent(-1, -1);
    PathNode start_node(start, 0, heuristic(start, goal), dummy_parent);

    open_set.push(start_node);
    g_costs[start] = 0;
    all_nodes[start] = start_node;

    constexpr int dx[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    constexpr int dy[] = { -1, 0, 1, -1, 1, -1, 0, 1 };

    while (!open_set.empty()) {
        PathNode curr = open_set.top();
        open_set.pop();
        Point curr_pos = curr.pos;

        if (curr.g_cost > g_costs[curr_pos]) continue;
        if (curr_pos == goal) {
            all_nodes[curr_pos] = curr;
            return reconstruct(goal, all_nodes);
        }

        for (int i = 0; i < 8; ++i) {
            Point neighbor_pos = { curr_pos.x + dx[i], curr_pos.y + dy[i] };
            if (!walkable(neighbor_pos, grid)) {
                continue;
            }

            int move_cost = (abs(dx[i]) + abs(dy[i]) == 2) ? DIAGONAL_COST : CARDINAL_COST;
            int tentative_g_cost = curr.g_cost + move_cost;

            // if better
            if (g_costs.find(neighbor_pos) == g_costs.end() || tentative_g_cost < g_costs[neighbor_pos]) {
                g_costs[neighbor_pos] = tentative_g_cost;
                int h = heuristic(neighbor_pos, goal);
                PathNode neighbor_node(neighbor_pos, tentative_g_cost, h, curr_pos);
                all_nodes[neighbor_pos] = neighbor_node;
                open_set.push(neighbor_node);
            }
        }
    }

    return {};
}

//static std::vector<Point> los_path(const std::vector<Point>& raw_path, const grid_t& grid) {
//    if (raw_path.size() <= 2) return raw_path;
//
//    std::vector<Point> simplified_path;
//    simplified_path.push_back(raw_path.front());
//
//    Point current_anchor = raw_path.front();
//    int i = 0;
//
//    while (i < raw_path.size() - 1) {
//        int j = i + 1;
//        int last_valid_index = i;
//
//        // Look ahead for the furthest waypoint reachable via a straight line
//        while (j < raw_path.size()) {
//            Point next_waypoint = raw_path[j];
//
//            // NOTE: is_line_obstructed must check all cells between anchor and waypoint
//            // accounting for entity size/radius.
//            if (grid.is_line_obstructed(current_anchor, next_waypoint)) {
//                // Obstruction found, the last valid waypoint (j-1) is the necessary corner.
//                break;
//            }
//            last_valid_index = j;
//            j++;
//        }
//
//        // Move the anchor to the last valid (non-obstructed) point
//        current_anchor = raw_path[last_valid_index];
//
//        // Add this necessary corner to the final path
//        if (simplified_path.back().x != current_anchor.x || simplified_path.back().y != current_anchor.y) {
//            simplified_path.push_back(current_anchor);
//        }
//
//        i = last_valid_index; // Continue search from the new anchor
//    }
//    return simplified_path;
//}

void AISystem::step(float elapsed_ms)
{
	float step_seconds = elapsed_ms / 1000.f;
	enemyStep(step_seconds);
	spriteStep(step_seconds); // Should be at the very end to overwrite motion
}

void AISystem::enemyStep(float step_seconds)
{
	Entity player = registry.players.entities[0];
	Motion& player_motion = registry.motions.get(player);
	auto& enemy_registry = registry.enemies;
	
	for(uint i = 0; i< enemy_registry.size(); i++) {
		Entity entity = enemy_registry.entities[i];
		Enemy& enemy = registry.enemies.get(entity);
		Motion& motion = registry.motions.get(entity);
		
		if (enemy.is_dead) {
			motion.angle += 3 * M_PI * step_seconds;
			motion.velocity = {0.0f, 0.0f};
			motion.scale -= glm::vec2(30.0f) * step_seconds;

			if (motion.scale.x < 0.f || motion.scale.y < 0.f) {
				registry.remove_all_components_of(entity);
			}
		} else {
            PathGrid& path_grid = registry.grids.components[0];
            std::vector<Point> pts = findPath(motion.position / 32.f, player_motion.position / 32.f, path_grid.grid);
            if (pts.size() < 2) continue;
			glm::vec2 diff = glm::ivec2(pts[1].x, pts[1].y) - glm::ivec2(pts[0].x, pts[0].y);
            //glm::vec2 diff = player_motion.position - motion.position;
			motion.angle = atan2(diff.y, diff.x);
			motion.velocity = glm::normalize(diff) * 50.f;

		}
	}
}

void AISystem::spriteStep(float step_seconds)
{
	auto& sprite_registry = registry.sprites;
	
	for(uint i = 0; i< sprite_registry.size(); i++) {
		Entity entity = sprite_registry.entities[i];
		Sprite& sprite = registry.sprites.get(entity);

		sprite.step_seconds_acc += step_seconds * 10.0f;
		sprite.curr_frame = (int)std::floor(sprite.step_seconds_acc) % sprite.total_frame;

		// Disable rotation for entity with sprites
		if (registry.motions.has(entity)) {
			Motion& motion = registry.motions.get(entity);
			if (motion.angle > M_PI_2 || motion.angle < -M_PI_2) {
				sprite.should_flip = true;
			} else {
				sprite.should_flip = false;
			}
			motion.angle = 0;
		}
	}
}