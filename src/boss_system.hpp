#pragma once

#include "common.hpp"
#include "render_system.hpp"
#include "tiny_ecs_registry.hpp"

class WorldSystem;

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

  float freq;
  float amp;
  float phase_offset;

  float root_angle;
};

void init(WorldSystem* world, RenderSystem* renderer, Entity player);
void startBossFight();
bool isBossFight();
void createCore(RenderSystem* renderer, vec2 pos);
void createBody(RenderSystem* renderer, vec2 pos);
void createTentacle(RenderSystem* renderer, vec2 root_pos, float direction);
void update(float dt_seconds);
void shutdown();

}
