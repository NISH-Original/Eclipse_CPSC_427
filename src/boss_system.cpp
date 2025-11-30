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
  t.bones.resize(8);
  t.segments.resize(8);

  for (int i = 0; i < 8; i++) {
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
    s.total_frame = 8;
    s.curr_frame = i;

    Motion& m = registry.motions.get(e);
    m.scale = vec2(128.f, 128.f);

    t.segments[i] = e;
  }

  for (int i = 0; i < 8; i++) {
    t.bones[i].local_angle = 0.f;
    t.bones[i].world_angle = 0.f;
    t.bones[i].length = 128.f;
    t.bones[i].parent = (i == 0 ? -1 : i - 1);
    t.bones[i].world_pos = root_pos;
  }

  g_tentacles.push_back(t);
}

static void updateTentacles(float dt) {
  for (auto& t : g_tentacles) {
    t.time += dt;

    for (int i = 0; i < 8; i++) {
      float phase = i * 0.4f;
      t.bones[i].local_angle = sin(t.time * 4.f + phase) * 0.3f;
    }

    for (int i = 0; i < 8; i++) {
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

    for (int i = 0; i < 8; i++) {
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
