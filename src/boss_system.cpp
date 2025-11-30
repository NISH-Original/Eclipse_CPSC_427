#include "boss_system.hpp"
#include "components.hpp"
#include "common.hpp"

namespace boss {

static std::vector<Tentacle> g_tentacles;

void init() {
}

void createTentacle(RenderSystem* renderer, vec2 root_pos) {
  Tentacle t;
  t.root_pos = root_pos;
  t.time = 0.f;
  t.bones.resize(16);
  t.segments.resize(16);

  for (int i = 0; i < 16; i++) {
    Entity e;

    Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
    registry.meshPtrs.emplace(e, &mesh);
    
    registry.motions.emplace(e);
    registry.sprites.emplace(e);
    registry.renderRequests.insert(
      e,
      { TEXTURE_ASSET_ID::BOSS_TENTACLE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE });
        
    Sprite& s = registry.sprites.get(e);
    s.total_row = 1;
    s.curr_row = 0;
    s.total_frame = 16;
    s.curr_frame = i;
    s.animation_enabled = false;

    Motion& m = registry.motions.get(e);
    m.scale = vec2(32.f, 32.f);

    t.segments[i] = e;
  }

  for (int i = 0; i < 16; i++) {
    t.bones[i].local_angle = 0.f;
    t.bones[i].world_angle = 0.f;
    t.bones[i].length = 26.f;
    t.bones[i].parent = (i == 0 ? -1 : i - 1);
    t.bones[i].world_pos = root_pos;
  }

  g_tentacles.push_back(t);
}

static void updateTentacles(float dt) {
  for (auto& t : g_tentacles) {
    t.time += dt;

    float frequency = 2.0f;
    float amplitude = 0.1f;

    for (int i = 0; i < 16; i++) {
      float phase = i * 0.25f;
      t.bones[i].local_angle = sin(t.time * frequency + phase) * amplitude;
    }

    for (int i = 0; i < 16; i++) {
      TentacleBone& b = t.bones[i];
      if (b.parent < 0) {
        b.world_angle = b.local_angle;
        b.world_pos = t.root_pos;
      } else {
        TentacleBone& p = t.bones[b.parent];
        b.world_angle = p.world_angle + b.local_angle;
        vec2 d = vec2(cos(p.world_angle), sin(p.world_angle));
        b.world_pos = p.world_pos + d * b.length;
      }
    }

    for (int i = 0; i < 16; i++) {
      Entity e = t.segments[i];
      Motion& m = registry.motions.get(e);
      m.position = t.bones[i].world_pos;
      m.angle = t.bones[i].world_angle;
    }
  }
}

void update(float dt_seconds) {
  updateTentacles(dt_seconds);
}

void shutdown() {
  g_tentacles.clear();
}

}
