#pragma once

#include <glm/vec2.hpp>

#include <limits>
#include <vector>

using PathVector = glm::ivec2;

constexpr int FIELD_RADIUS = 16;
constexpr int FIELD_SIZE = FIELD_RADIUS * 2 + 1;
constexpr PathVector DIRECTIONS[] = {
	{ 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 },
	{ 1, 1 }, { -1, 1 }, { -1, -1 }, { 1, -1 }
};
constexpr int CARDINAL_COST = 10;
constexpr int DIAGONAL_COST = 14;

struct PathNode {
	int cost = std::numeric_limits<int>::max();
	PathVector dir{ 0, 0 };
	bool walkable = true;
};

class PathfindingSystem {
public:
	PathfindingSystem() : flow_field(FIELD_SIZE, std::vector<PathNode>(FIELD_SIZE)) {}

	void step(float elapsed_ms);

private:
	std::vector<std::vector<PathNode>> flow_field;

	void build_flow_field();
	void add_path_force();
};