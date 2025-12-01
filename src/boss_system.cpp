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

static Entity hitbox;
static Entity core;
static Entity body; 
static std::vector<Tentacle> g_tentacles;

static float core_time;
static bool is_boss_fight = false;

static vec2 player_prev_pos;
static float squeeze_cooldown = 0.f;

static bool core_dead;

static float cone_damage_timer = 0.f;
static float boss_attack_timer = 0.f;
static int boss_attack_state = 0;
static float spin_angle = 0.f;

static float frenzy_t = 0.f;
static float spin_speed = 0.f;

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
  core_dead = false;

  for (int i = registry.enemies.size() - 1; i >= 0; i--) {
    registry.remove_all_components_of(registry.enemies.entities[i]);
  }

  registry.serial_chunks.clear();
  registry.chunks.clear();
  while (!registry.obstacles.entities.empty()) {
    Entity obstacle = registry.obstacles.entities.back();
    registry.remove_all_components_of(obstacle);
  }

  createHitbox(renderer, center);
	createBody(renderer, root_pos);
  createTentacle(renderer, center + vec2(cos(0.f), sin(0.f)) * 30.f, 0.f);
  createTentacle(renderer, center + vec2(cos(M_PI), sin(M_PI)) * 30.f, M_PI);
  createTentacle(renderer, center + vec2(cos(-M_PI / 2.f), sin(-M_PI / 2.f)) * 30.f, -M_PI / 2.f);
  createTentacle(renderer, center + vec2(cos(M_PI / 2.f), sin(M_PI / 2.f)) * 30.f, M_PI / 2.f);
  createTentacle(renderer, center + vec2(cos(-M_PI / 4.f), sin(-M_PI / 4.f)) * 30.f, -M_PI / 4.f);
  createTentacle(renderer, center + vec2(cos(M_PI / 4.f), sin(M_PI / 4.f)) * 30.f, M_PI / 4.f);
  createTentacle(renderer, center + vec2(cos(-3.f * M_PI / 4.f), sin(-3.f * M_PI / 4.f)) * 30.f, -3.f * M_PI / 4.f);
  createTentacle(renderer, center + vec2(cos(3.f * M_PI / 4.f), sin(3.f * M_PI / 4.f)) * 30.f, 3.f * M_PI / 4.f);
	createCore(renderer, core_pos);

  Entity core_entity = core;
  registry.renderRequests.remove(core_entity);
  registry.renderRequests.insert(core_entity,
      { TEXTURE_ASSET_ID::BOSS_CORE,
        EFFECT_ASSET_ID::TEXTURED,
        GEOMETRY_BUFFER_ID::SPRITE });

  Motion& pm = registry.motions.get(player);
  pm.position = {center.x, center.y + window_width_px / 8.f};
  player_prev_pos = pm.position;

  renderer->setCameraPosition(center);
}

bool isBossFight() {
  return is_boss_fight;
}

void createHitbox(RenderSystem* renderer, vec2 pos) {
  hitbox = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(hitbox, &mesh);

	Motion& motion = registry.motions.emplace(hitbox);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 0.f, 0.f }; // Scale based on mesh original size

  Sprite& sprite = registry.sprites.emplace(hitbox);
	sprite.total_row = 1;
	sprite.total_frame = 1;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;

  Enemy& enemy = registry.enemies.emplace(hitbox);
	enemy.health = 50; // Boss health
  enemy.max_health = 0;
	enemy.damage = 25;
	enemy.xylarite_drop = 1;

  StationaryEnemy& se = registry.stationaryEnemies.emplace(hitbox);
	se.position = pos;

	registry.collisionCircles.emplace(hitbox).radius = 20.f;
	registry.boss_parts.emplace(hitbox);
  
  registry.renderRequests.insert(
  hitbox,
  { TEXTURE_ASSET_ID::ENEMY1,
    EFFECT_ASSET_ID::TEXTURED,
    GEOMETRY_BUFFER_ID::SPRITE });

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
	registry.boss_parts.emplace(core);

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
  t.amp  = frand(0.08f, 0.14f);
  t.amp *= 1.2f;

  t.base_freq = t.freq;
  t.base_amp  = t.amp;
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
	  registry.boss_parts.emplace(e);

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

void updatePlayerOutOfBounds(float dt) {
  Motion& pm = registry.motions.get(player);

  float half_w = pm.scale.x * 0.5f;
  float half_h = pm.scale.y * 0.5f;

  if (pm.position.x - half_w < 0.f)
    pm.position.x = half_w;

  if (pm.position.x + half_w > window_width_px)
    pm.position.x = window_width_px - half_w;

  if (pm.position.y - half_h < 280.f)
    pm.position.y = 280 + half_h;

  if (pm.position.y + half_h >  280.f + window_height_px)
    pm.position.y = 280.f + window_height_px - half_h;
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
   
    bool player_died = world->on_player_hit(10, center);	
    if (player_died) {
      world->handle_player_death();
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

  if (!core_dead) {
    Boss& b = registry.boss_parts.get(core);
    Enemy& e = registry.enemies.get(hitbox);
    b.is_hurt = e.is_hurt;
    if (e.is_dead) {
      core_dead = true;
      registry.remove_all_components_of(hitbox);
      for (int ti = 0; ti < g_tentacles.size(); ti++) {
       Tentacle& t = g_tentacles[ti];
        t.health = 0;
      }
    }
  } 
}

static void updateTentacles(float dt) {
  bool core_hurt = false;
  if (registry.boss_parts.has(core)) {
    Boss& cb = registry.boss_parts.get(core);
    core_hurt = cb.is_hurt;
  }

  for (int ti = 0; ti < g_tentacles.size(); ti++) {
    Tentacle& t = g_tentacles[ti];

    if (core_hurt && !t.is_dying) {
      t.is_hurt = true;
      if (t.hurt_time < 0.2f)
        t.hurt_time = 0.2f;

      for (int i = 0; i < (int)t.segments.size(); i++) {
        Entity e = t.segments[i];
        if (registry.boss_parts.has(e)) {
          Boss& b = registry.boss_parts.get(e);
          b.is_hurt = true;
        }
      }
    }

    if (t.health <= 0 && !t.is_dying) {
      t.is_dying = true;
      t.death_timer = 1.0f;
      boss_attack_state = 1;
      boss_attack_timer = 0.f;

      for (int i = 0; i < 16; i++) {
        Entity e = t.segments[i];

        if (registry.obstacles.has(e))
            registry.obstacles.remove(e);

        Motion& m = registry.motions.get(e);
        vec2 player_pos = registry.motions.get(player).position;
        vec2 base_dir = m.position - player_pos;

        float len = sqrt(base_dir.x * base_dir.x + base_dir.y * base_dir.y);
        if (len > 0.001f) {
            base_dir.x /= len;
            base_dir.y /= len;
        } else {
            base_dir = vec2(1.f, 0.f);
        }

        float rand_angle = frand(-0.6f, 0.6f);
        float angle = atan2(base_dir.y, base_dir.x) + rand_angle;

        float speed = frand(120.f, 240.f);

        m.velocity = vec2(cos(angle) * speed, sin(angle) * speed);
      }
    }

    if (t.is_dying) {
      t.death_timer -= dt;

      for (int i = 0; i < 16; i++) {
        Entity e = t.segments[i];
        Motion& m = registry.motions.get(e);

        m.scale *= (1.f - dt * 1.5f);
        if (m.scale.x < 0.f) m.scale.x = 0.f;
        if (m.scale.y < 0.f) m.scale.y = 0.f;

        m.position += m.velocity * dt;
      }

      if (t.death_timer <= 0.f) {
        for (int i = 0; i < 16; i++) {
            registry.remove_all_components_of(t.segments[i]);
        }
        g_tentacles.erase(g_tentacles.begin() + ti);
        ti--;
      }

      continue;
    }

    if (t.is_hurt) {
      t.hurt_time -= dt;
      if (t.hurt_time <= 0.f) {
        t.is_hurt = false;
        for (int i = 0; i < 16; i++) {
          Entity e = t.segments[i];
          Boss& b = registry.boss_parts.get(e);
          b.is_hurt = false;
        }
      }
    }

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

    if (!t.is_hurt) {
      for (int i = 0; i < 16; i++) {
        Entity ei = t.segments[i];
        Boss& bi = registry.boss_parts.get(ei);
        if (bi.is_hurt) {
          t.health -= 50;
          t.is_hurt = true;
          t.hurt_time = 0.2f;
          for (int j = 0; j < 16; j++) {
            Entity ej = t.segments[j];
            Boss& bj = registry.boss_parts.get(ej);
            bj.is_hurt = true;
          }
          break;
        }
      }
    }
  }
}

bool pointInCone(vec2 point, vec2 origin, vec2 dir, float coneAngle, float minR, float coneLength) {
  vec2 v = point - origin;
  float dist = sqrt(v.x*v.x + v.y*v.y);
  if (dist < minR || dist > coneLength)
    return false;

  vec2 nv = v / dist;
  float d = nv.x*dir.x + nv.y*dir.y;
  float angle = acos(d);
  return angle < coneAngle * 0.5f;
}

static bool buff_applied = false;

static void attackUpdate(float dt) {
  if (boss_attack_state == 0)
    return;

  boss_attack_timer += dt;

  if (boss_attack_state == 1) {
    frenzy_t += dt * 2.0f;
    if (frenzy_t > 1.f) frenzy_t = 1.f;

    for (Tentacle& t : g_tentacles) {
      float k = frenzy_t;
      t.freq = t.base_freq * (1.f + k * 1.5f);
      t.amp  = t.base_amp  * (1.f + k * 1.5f);
    }

    if (boss_attack_timer >= 3.f) {
      boss_attack_state = 2;
      boss_attack_timer = 0.f;
      cone_damage_timer = 0.f;

      Motion& cm = registry.motions.get(core);
      Motion& pm = registry.motions.get(player);
      vec2 origin = cm.position;
      vec2 playerPos = pm.position;

      vec2 diff = playerPos - origin;
      float playerAngle = atan2(diff.y, diff.x);

      float offset = ((rand() / (float)RAND_MAX) - 0.5f) * (M_PI / 3.f);
      float startAngle = playerAngle + offset;

      spin_angle = startAngle;

      if (startAngle > playerAngle)
        spin_speed = -0.6f;
      else
        spin_speed = +0.6f;
    }
    return;
  }

  if (boss_attack_state == 2) {
    Motion& cm = registry.motions.get(core);
    vec2 origin = cm.position;

    spin_angle += spin_speed * dt;
    vec2 dir = normalize(vec2(cos(spin_angle), sin(spin_angle)));

    float tt = fmod(boss_attack_timer * 0.5f, 3.f);
    vec4 col;

    if (tt < 1.f) {
      float k = tt;
      col = vec4(
        0.25f + (0.f   - 0.25f) * k,
        0.55f + (1.f   - 0.55f) * k,
        1.f   + (1.f   - 1.f)   * k,
        1.f
      );
    }
    else if (tt < 2.f) {
      float k = tt - 1.f;
      col = vec4(
        0.f   + (0.80f - 0.f)   * k,
        1.f   + (0.35f - 1.f)   * k,
        1.f   + (1.f   - 1.f)   * k,
        1.f
      );
    }
    else {
      float k = tt - 2.f;
      col = vec4(
        0.80f + (0.25f - 0.80f) * k,
        0.35f + (0.55f - 0.35f) * k,
        1.f   + (1.f   - 1.f)   * k,
        1.f
      );
    }

    createBeamParticlesCone(origin, dir, 20, col);

    Motion& pm = registry.motions.get(player);
    vec2 playerPos = pm.position;

    float coneLen = window_width_px * 0.7f;
    float minR = coneLen * 0.05f;

    cone_damage_timer += dt;
    if (cone_damage_timer >= 0.2f) {
      if (pointInCone(playerPos, origin, dir, 0.3f, minR, coneLen)) {
        Player& p = registry.players.get(player);
        p.health -= 20;
      }
      cone_damage_timer = 0.f;
    }

    if (boss_attack_timer >= 10.f) {
      frenzy_t = 0.f;
      for (Tentacle& t : g_tentacles) {
        t.freq = t.base_freq;
        t.amp  = t.base_amp;
      }
      boss_attack_state = 0;
      boss_attack_timer = 0.f;
    }
  }
}


void update(float dt_seconds) {
  static float blood_time = 0.f;
  static bool prev_core_dead = false;
  static bool saved = false;

  static vec2 core_initial_scale;
  static vec2 body_initial_scale;
  static vec2 core_initial_pos;
  static vec2 body_initial_pos;

  static float total_drops_xylarite = 0.f;
  static float total_drops_firstaid = 0.f;

  if (!saved && registry.motions.has(core) && registry.motions.has(body)) {
    core_initial_scale = registry.motions.get(core).scale;
    body_initial_scale = registry.motions.get(body).scale;
    core_initial_pos = registry.motions.get(core).position;
    body_initial_pos = registry.motions.get(body).position;
    saved = true;
  }

  updateCore(dt_seconds);
  
  if (!core_dead) {
    updatePlayerSqueezed(dt_seconds);
    updatePlayerOutOfBounds(dt_seconds);
    attackUpdate(dt_seconds);
  }

  updateTentacles(dt_seconds);

  Player& p = registry.players.get(player);
  if (p.health <= 0) {
    world->handle_player_death();
  }

  float shrink = 1.f;
  float fall_offset = 0.f;

  if (core_dead) {
    if (!prev_core_dead) {
      blood_time = 0.f;
      total_drops_xylarite = 0.f;
      total_drops_firstaid = 0.f;
    }

    blood_time += dt_seconds;

    if (blood_time < 3.f)
      shrink = 1.f;
    else {
      float t = (blood_time - 3.f) / 7.f;
      shrink = max(0.f, 1.f - t);
    }

    fall_offset = (1.f - shrink) * 80.f;

    Motion& core_m = registry.motions.get(core);
    Motion& body_m = registry.motions.get(body);

    if (shrink <= 0.5f)
      core_m.scale = vec2(0.f, 0.f);
    else
      core_m.scale = core_initial_scale * shrink;

    body_m.scale = body_initial_scale * shrink;

    core_m.position = core_initial_pos + vec2(0.f, fall_offset);
    body_m.position = body_initial_pos + vec2(0.f, fall_offset);

    float blood_rate_factor;
    if (shrink > 0.f)
      blood_rate_factor = 1.f;
    else {
      float t = clamp((blood_time - 10.f) / 2.f, 0.f, 1.f);
      blood_rate_factor = 1.f - t;
    }

    if (!(shrink == 0.f && blood_time > 12.f)) {
      float base = 5000.f * dt_seconds * blood_rate_factor;
      float noise = ((float)rand() / RAND_MAX - 0.5f) * 80.f;
      int count = max(0, (int)(base + noise));
      createBossBloodParticles(center + vec2(0.f, fall_offset), count);
    }

    if (shrink > 0.f) {
      total_drops_xylarite += dt_seconds * (50.f / 7.f);
      while (total_drops_xylarite >= 1.f) {
        total_drops_xylarite -= 1.f;
        float a = ((float)rand() / RAND_MAX) * 2.f * M_PI;
        float r = ((float)rand() / RAND_MAX) * 250.f;
        vec2 p = vec2(center.x + cos(a) * r, center.y + sin(a) * r);
        createXylarite(renderer, p);
      }

      total_drops_firstaid += dt_seconds * (2.f / 7.f);
      while (total_drops_firstaid >= 1.f) {
        total_drops_firstaid -= 1.f;
        float a = ((float)rand() / RAND_MAX) * 2.f * M_PI;
        float r = ((float)rand() / RAND_MAX) * 250.f;
        vec2 p = vec2(center.x + cos(a) * r, center.y + sin(a) * r);
        createFirstAid(renderer, p);
      }
    }
  } else {
    registry.renderRequests.remove(core);
    registry.renderRequests.insert(core,
        { TEXTURE_ASSET_ID::BOSS_CORE,
          EFFECT_ASSET_ID::TEXTURED,
          GEOMETRY_BUFFER_ID::SPRITE });
  }

  prev_core_dead = core_dead;
  renderer->setCameraPosition(center);
}

void shutdown() {
  is_boss_fight = false;

  registry.remove_all_components_of(core);
  registry.remove_all_components_of(body);
  registry.remove_all_components_of(hitbox);

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
