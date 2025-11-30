#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tiny_ecs_registry.hpp"

namespace boss {

struct TentacleBone {
  float local_angle;
  float world_angle;
  float length;
  int parent;
  vec2 world_pos;
};

struct Tentacle {
  std::vector<Entity> segments;
  std::vector<TentacleBone> bones;
  vec2 root_pos;
  float time;
};

void init();
void createTentacle(RenderSystem* renderer, vec2 root_pos);
void update(float dt_seconds);
void shutdown();

}
