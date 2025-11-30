#include "boss_system.hpp"
#include "world_system.hpp"
#include "components.hpp"
#include "common.hpp"
#include <iostream>

namespace boss {

static const vec2 center = {window_width_px / 2.f, window_width_px / 2.f};
static const vec2 root_pos = { center.x, center.y + 64 };
static const vec2 core_pos = { center.x, center.y - 16 };

static WorldSystem* world;
static RenderSystem* renderer;
static Entity player;

static Entity core;
static Entity body; 
static std::vector<Tentacle> g_tentacles;

static float core_time;
static bool is_boss_fight = false;

static vec2 player_prev_pos;
static float squeeze_cooldown = 0.f;

static float frand(float a, float b) {
  return a + (b - a) * ((float)rand() / RAND_MAX);
}

void init(WorldSystem* w, RenderSystem* r, Entity p) {
  world = w;
  renderer = r;
  player = p;
}

void startBossFight() {
  shutdown();
  is_boss_fight = true;

  for (int i = registry.enemies.size() - 1; i >= 0; i--) {
    registry.remove_all_components_of(registry.enemies.entities[i]);
  }

  registry.serial_chunks.clear();
  registry.chunks.clear();
  while (!registry.obstacles.entities.empty()) {
    Entity obstacle = registry.obstacles.entities.back();
    registry.remove_all_components_of(obstacle);
  }

	createBody(renderer, root_pos);
	createTentacle(renderer, center, 0.f);
	createTentacle(renderer, center, M_PI);
	createTentacle(renderer, center, -M_PI / 2.f);
	createTentacle(renderer, center, M_PI / 2.f);
	createTentacle(renderer, center, -M_PI / 4.f);
	createTentacle(renderer, center, M_PI / 4.f);
	createTentacle(renderer, center, -3.f * M_PI / 4.f);
	createTentacle(renderer, center, 3.f * M_PI / 4.f);
	createCore(renderer, core_pos);

  Motion& pm = registry.motions.get(player);
  pm.position = {center.x, center.y + window_width_px / 8.f};
  player_prev_pos = pm.position;

  renderer->setCameraPosition(center);
}

bool isBossFight() {
  return is_boss_fight;
}

void createCore(RenderSystem* renderer, vec2 pos) {
  core_time = 0.0f;
	core = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(core, &mesh);

	Motion& motion = registry.motions.emplace(core);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 128.f, 128.f };

	Sprite& sprite = registry.sprites.emplace(core);
	sprite.total_row = 1;
	sprite.total_frame = 1;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;

	registry.nonColliders.emplace(core);

	registry.renderRequests.insert(
		core,
		{ TEXTURE_ASSET_ID::BOSS_CORE,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE }
  );
}

void createBody(RenderSystem* renderer, vec2 pos) {
	body = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(body, &mesh);

	Motion& motion = registry.motions.emplace(body);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 256.f, 128.f };

	Sprite& sprite = registry.sprites.emplace(body);
	sprite.total_row = 1;
	sprite.total_frame = 1;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;
  sprite.animation_enabled = false;

	registry.nonColliders.emplace(body);

	registry.renderRequests.insert(
		body,
		{ TEXTURE_ASSET_ID::BOSS_BODY,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE }
  );
}

void createTentacle(RenderSystem* renderer, vec2 root_pos, float direction) {
  Tentacle t;
  t.root_pos = root_pos;
  t.time = 0.f;

  t.freq = frand(1.5f, 3.0f);
  t.amp = frand(0.08f, 0.14f);
  t.phase_offset = frand(0.f, 10.f);

  t.root_angle = direction;

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
    m.scale = vec2(16.f, 16.f);

	  registry.obstacles.emplace(e);

    t.segments[i] = e;
  }

  for (int i = 0; i < 16; i++) {
    t.bones[i].local_angle = 0.f;
    t.bones[i].world_angle = 0.f;
    t.bones[i].length = 13.f;
    t.bones[i].parent = (i == 0 ? -1 : i - 1);
    t.bones[i].world_pos = root_pos;
  }

  g_tentacles.push_back(t);
}

void updatePlayerSqueezed(float dt) {
  Motion& pm = registry.motions.get(player);

  
  if (squeeze_cooldown > 0.f) {
      squeeze_cooldown -= dt;
      if (squeeze_cooldown < 0.f)
          squeeze_cooldown = 0.f;

      player_prev_pos = pm.position;
      return;
  }

  float dx = pm.position.x - player_prev_pos.x;
  float dy = pm.position.y - player_prev_pos.y;

  float dist2 = dx*dx + dy*dy;
  float threshold = 25.f * 25.f;

  if (dist2 > threshold) {
    squeeze_cooldown = 1.f;
    Player& p = registry.players.get(player);
    p.health -= 10;

    Motion& player_motion = registry.motions.get(player);
    vec2 direction = player_motion.position - center;
    float dir_len = sqrtf(direction.x * direction.x + direction.y * direction.y);
    if (dir_len > 0.0001f) {
      world->hurt_knockback_direction.x = direction.x / dir_len;
      world->hurt_knockback_direction.y = direction.y / dir_len;
      world->is_hurt_knockback = true;
      world->hurt_knockback_timer = world->hurt_knockback_duration;
      
      // store current animation before hurt
      if (registry.sprites.has(player)) {
        Sprite& sprite = registry.sprites.get(player);
        if (sprite.is_reloading || sprite.is_shooting) {
          world->animation_before_hurt = sprite.previous_animation;
          sprite.is_shooting = false;
        } else {
          world->animation_before_hurt = sprite.current_animation;
        }
      }
    }
  }

  player_prev_pos = pm.position;
}

static void updateCore(float dt) {
  core_time += dt;

  if(registry.motions.has(core)) {
    Motion& m = registry.motions.get(core);

    float t = core_time;

    float s = sin(t * 3.0f);
    float base = 128.f;
    float amp  = 8.f;

    m.scale.x = base + s * amp;
    m.scale.y = base - s * amp;
  }
}

static void updateTentacles(float dt) {
  for (auto& t : g_tentacles) {
    t.time += dt;

    for (int i = 0; i < 16; i++) {
      float local_phase = i * 0.25f;

      t.bones[i].local_angle =
        sin((t.time + t.phase_offset) * t.freq + local_phase) * t.amp;
    }

    for (int i = 0; i < 16; i++) {
      TentacleBone& b = t.bones[i];
      if (b.parent < 0) {
        b.world_angle = t.root_angle + b.local_angle;
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
  updateCore(dt_seconds);
  updateTentacles(dt_seconds);
  updatePlayerSqueezed(dt_seconds);
}

void shutdown() {
  is_boss_fight = false;

  registry.remove_all_components_of(core);
  registry.remove_all_components_of(body);

  for (size_t i = 0; i < g_tentacles.size(); i++) {
    Tentacle& t = g_tentacles[i];
    for (size_t j = 0; j < t.segments.size(); j++) {
        Entity e = t.segments[j];
        registry.remove_all_components_of(e);
    }
  }

  g_tentacles.clear();
}

}
