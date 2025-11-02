#pragma once

constexpr float ROTATE_EPSILON = 0.001f;

class SteeringSystem {
public:
	void step(float elapsed_ms);
};