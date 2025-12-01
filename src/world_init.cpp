#include "common.hpp"
#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"
#include <utility>

Entity createPlayer(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 50.f;

	// Create sprite component for animation
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = sprite.idle_frames; // Start with idle animation
	sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;

	// add collision mesh for player (for damage detection only)
	{
		CollisionMesh& col = registry.colliders.emplace(entity);
		col.local_points = {
			{ -0.29f, -0.26f }, { -0.29f,  0.24f }, { -0.19f,  0.29f }, {  0.11f,  0.29f },
			{  0.21f,  0.24f }, {  0.45f,  0.24f }, {  0.45f,  0.14f }, {  0.26f,  0.14f },
			{  0.31f, -0.15f }, {  0.01f, -0.26f }, {  0.01f, -0.36f }
		};
	}

	// add collision circle for player (for blocking/pushing)
	{
		vec2 bb = { abs(motion.scale.x), abs(motion.scale.y) };
		float radius = sqrtf(bb.x*bb.x + bb.y*bb.y) / 5.f;
		registry.collisionCircles.emplace(entity).radius = radius - 3;
	}

	// create an empty Salmon component for our character
	registry.players.emplace(entity);
	// Constrain player to screen boundaries
	//registry.constrainedEntities.emplace(entity);

	// Add a radial light to the player
	Light& player_light = registry.lights.emplace(entity);
	player_light.light_color = vec3(0.6f, 0.55f, 0.45f);
	player_light.follow_target = Entity();
	player_light.offset = vec2(0.0f, 0.0f);
	player_light.range = 500.0f;
	player_light.cone_angle = 3.14159f;
	player_light.brightness = 2.0f;
	player_light.use_target_angle = false;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PLAYER_IDLE,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE }); // use sprite geometry instead of player circle

	return entity;
}

Entity createFeet(RenderSystem* renderer, vec2 pos, Entity parent_player)
{
	auto entity = Entity();
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 45.f;

    // sprite component for animation
    Sprite& sprite = registry.sprites.emplace(entity);
		sprite.total_row = 1;
    sprite.total_frame = 20; // Feet walk has 20 frames
    sprite.current_animation = TEXTURE_ASSET_ID::FEET_WALK;

	// feet component
	Feet& feet = registry.feet.emplace(entity);
	feet.parent_player = parent_player;
	feet.transition_pending = false;
	feet.transition_target = TEXTURE_ASSET_ID::FEET_WALK;
	feet.transition_frame_primary = -1;
	feet.transition_frame_secondary = -1;
	feet.transition_start_frame = 0;
	feet.last_horizontal_sign = 0;
	feet.locked_horizontal_texture = TEXTURE_ASSET_ID::FEET_WALK;
	feet.locked_texture_valid = false;

    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::FEET_WALK,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createDash(RenderSystem* renderer, vec2 pos, Entity parent_player)
{
	auto entity = Entity();
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// initial values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = {0.0f, 0.0f}; // start hidden, only visible when dashing

	// sprite component
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1; // single frame sprite
	sprite.current_animation = TEXTURE_ASSET_ID::DASH;

	// dash component
	Feet& dash = registry.feet.emplace(entity);
	dash.parent_player = parent_player;
	dash.transition_pending = false;
	dash.transition_target = TEXTURE_ASSET_ID::FEET_WALK;
	dash.transition_frame_primary = -1;
	dash.transition_frame_secondary = -1;
	dash.transition_start_frame = 0;
	dash.last_horizontal_sign = 0;
	dash.locked_horizontal_texture = TEXTURE_ASSET_ID::FEET_WALK;
	dash.locked_texture_valid = false;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::DASH,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createTree(RenderSystem* renderer, vec2 pos, float scale)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * scale; // Scale based on mesh original size

	// create component for our tree
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;

	registry.obstacles.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TREE,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createWall(RenderSystem* renderer, vec2 pos, vec2 scale)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = vec2(mesh.original_size.x * scale.x, mesh.original_size.y * scale.y);

	// create component for our wall
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;

	registry.obstacles.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::WALL,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createBonfire(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 100.f;

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 6;
	sprite.should_flip = false;
	sprite.animation_speed = 5.0f;

	registry.obstacles.emplace(entity);
	registry.collisionCircles.emplace(entity).radius = 50.f;

	registry.lights.emplace(entity);
	Light& light = registry.lights.get(entity);
	light.is_enabled = true;
	light.light_color = { 1.0f, 0.5f, 0.1f };
	light.brightness = 1.5f;
	light.range = 400.0f;
	light.cone_angle = 3.14159f;
	light.use_target_angle = false;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::BONFIRE,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createArrow(RenderSystem* renderer)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = { 0.f, 0.f }; // Will be updated to camera position each frame
	motion.angle = 0.f; // Will be updated to point toward bonfire each frame
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 150.f; // Slightly smaller than player

	// Create sprite component (required for TEXTURED effect rendering)
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1; // Arrow is a static sprite, no animation
	sprite.should_flip = false;

	// Create arrow component
	registry.arrows.emplace(entity);

	// Add render request (same pattern as bonfire and player)
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::ARROW,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

void createBloodParticles(vec2 pos, vec2 bullet_vel, int count) {
  for (int i = 0; i < count; i++) {
    auto entity = Entity();
    Particle& p = registry.particles.emplace(entity);

    float ox = ((rand() / (float)RAND_MAX) - 0.5f) * 10.f;
    float oy = ((rand() / (float)RAND_MAX) - 0.5f) * 10.f;
    p.position = vec3(pos.x + ox, pos.y + oy, 0);

    float base = atan2(bullet_vel.y, bullet_vel.x);

    float r = (rand() / (float)RAND_MAX);
    float offset = (r * r) * 0.523599f;
    if (rand() % 2 == 0) offset = -offset;

    float angle = base + offset;
    vec2 dir = normalize(vec2(cos(angle), sin(angle)));

    float speed = 200.f + (rand() / (float)RAND_MAX) * 150.f;

    p.velocity = vec3(dir.x * speed, dir.y * speed, 0);
    p.color = vec4(0.7f, 0.05f, 0.05f, 1.f);
    p.size = 8.f;

    p.lifetime = 0.3f + (rand() / (float)RAND_MAX) * 0.3f;
    p.age = 0.f;
    p.alive = true;
  }
}

void createBossBloodParticles(vec2 pos, int count) {
  for (int i = 0; i < count; i++) {
    auto entity = Entity();
    Particle& p = registry.particles.emplace(entity);

    float radius = 20.f + ((float)rand() / RAND_MAX) * 20.f;
    float a = ((float)rand() / RAND_MAX) * 2.f * M_PI;
    float r = radius * sqrt((float)rand() / RAND_MAX);

    float ox = cos(a) * r;
    float oy = sin(a) * r;
    p.position = vec3(pos.x + ox, pos.y + oy, 0);

    float angle = ((float)rand() / RAND_MAX) * 2.f * M_PI;
    vec2 dir = normalize(vec2(cos(angle), sin(angle)));

    float speed = 150.f + ((float)rand() / RAND_MAX) * 400.f;
    p.velocity = vec3(dir.x * speed, dir.y * speed, 0);

    float size = 10.f + ((float)rand() / RAND_MAX) * 10.f;
    p.size = size;

    float lifetime = 0.3f + ((float)rand() / RAND_MAX) * 0.8f;
    p.lifetime = lifetime;

    float c = 0.7f + ((float)rand() / RAND_MAX) * 0.1f;
    p.color = vec4(c, 0.05f, 0.05f, 1.f);

    p.age = 0.f;
    p.alive = true;
  }
}

vec2 randomPointInCone(vec2 origin, vec2 dir, float coneAngle, float minRadius, float coneLength) {
  float a = atan2(dir.y, dir.x);
  float offset = ((rand() / (float)RAND_MAX) - 0.5f) * coneAngle;
  float angle = a + offset;
  float r = minRadius + (rand() / (float)RAND_MAX) * (coneLength - minRadius);
  float x = origin.x + cos(angle) * r;
  float y = origin.y + sin(angle) * r;
  return vec2(x, y);
}

void createBeamParticlesCone(vec2 origin, vec2 dir, int count, vec4 col) {
  float coneLen = window_width_px * 0.7f;
  float minR = coneLen * 0.05f;
  int real_count = count * 15;
  for (int i = 0; i < real_count; i++) {
    auto entity = Entity();
    Particle& p = registry.particles.emplace(entity);

    vec2 pos = randomPointInCone(origin, dir, 0.3f, minR, coneLen);
    float z = ((rand() / (float)RAND_MAX) - 0.5f) * 0.001f;
    p.position = vec3(pos.x, pos.y, z);

    vec2 v = normalize(pos - origin);
    float speed = 4.f + (rand() / (float)RAND_MAX) * 4.f;
    p.velocity = vec3(v.x * speed, v.y * speed, 0);

    p.color = col;
    p.size = 7.f;

    p.lifetime = 0.5f;
    p.age = 0.f;
    p.alive = true;
  }
}

void createDashParticles(vec2 pos, vec2 dash_dir) {
  vec2 opposite_dir = -dash_dir;
  
  int particle_count = 6 + (rand() % 5); // 6-10 particles per call (increased from 3-5)
  
  for (int i = 0; i < particle_count; i++) {
    auto entity = Entity();
    Particle& p = registry.particles.emplace(entity);
    
    // Position particles behind the player with some spread
    float offset_dist = 20.f + ((rand() / (float)RAND_MAX) * 30.f);
    float spread_angle = ((rand() / (float)RAND_MAX) - 0.5f) * 0.4f;
    
    float angle = atan2(opposite_dir.y, opposite_dir.x) + spread_angle;
    float ox = cos(angle) * offset_dist;
    float oy = sin(angle) * offset_dist;
    
    // Add some perpendicular spread
    float perp_angle = angle + M_PI / 2.0f;
    float perp_spread = ((rand() / (float)RAND_MAX) - 0.5f) * 15.f;
    ox += cos(perp_angle) * perp_spread;
    oy += sin(perp_angle) * perp_spread;
    
    p.position = vec3(pos.x + ox, pos.y + oy, 0);
    
    // Velocity points backward (opposite of dash direction) with some randomness
    vec2 vel_dir = normalize(vec2(cos(angle), sin(angle)));
    float speed = 50.f + (rand() / (float)RAND_MAX) * 100.f;
    p.velocity = vec3(vel_dir.x * speed, vel_dir.y * speed, 0);
    
    float blue_intensity = 0.6f + (rand() / (float)RAND_MAX) * 0.4f; // 0.6 to 1.0
    float green_tint = 0.3f + (rand() / (float)RAND_MAX) * 0.3f; // 0.3 to 0.6 for cyan-blue
    p.color = vec4(0.2f, green_tint, blue_intensity, 1.f);
    
    // Size variation
    p.size = 6.f + (rand() / (float)RAND_MAX) * 8.f; // 6 to 14
    
    // Lifetime - particles fade out
    p.lifetime = 0.3f + (rand() / (float)RAND_MAX) * 0.2f; // 0.3 to 0.5 seconds
    p.age = 0.f;
    p.alive = true;
  }
}


Entity createXylarite(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = {25.0f, 25.0f};

	// create component for our tree
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;

	registry.drops.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::XYLARITE,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createFirstAid(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = {50.0f, 50.0f};

	// create component for our tree
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;

	registry.drops.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::FIRST_AID,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity create_drop_trail(const Motion& src_motion, const Sprite& src_sprite) {
    auto entity = Entity();

    Motion& m = registry.motions.emplace(entity);
    m.position = src_motion.position;
    m.angle = src_motion.angle;
    m.scale = src_motion.scale * 0.85f;
    m.velocity = {0.f, 0.f};

    Sprite& s = registry.sprites.emplace(entity);
    s = src_sprite;

    Trail& t = registry.trails.emplace(entity);
    t.life = 0.25f;
    t.alpha = 0.5f;

    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::TRAIL,
          EFFECT_ASSET_ID::TRAIL,
          GEOMETRY_BUFFER_ID::SPRITE });

    return entity;
}

Entity createEnemy(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 100.f, 100.f };

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;

	Enemy& enemy =registry.enemies.emplace(entity);
	
	// Base stats for basic enemy type
	int base_health = 100;
	int base_damage = 10;
	
	float health_multiplier = level_manager.get_enemy_health_multiplier(level, time_in_level_seconds);
	int final_health = static_cast<int>(base_health * health_multiplier);
	
	float damage_multiplier = level_manager.get_enemy_damage_multiplier(level, time_in_level_seconds);
	int final_damage = static_cast<int>(base_damage * damage_multiplier);
	
	enemy.health = final_health;
	enemy.max_health = final_health;
	enemy.damage = final_damage;
	enemy.xylarite_drop = level;

	registry.collisionCircles.emplace(entity).radius = 40.f;

	MovementAnimation& anim = registry.movementAnimations.emplace(entity);
	anim.base_scale = { 100.f, 100.f };

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::ENEMY1,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createMinion(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 25.f, 25.f };

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;

	Enemy& enemy = registry.enemies.emplace(entity);
	
	enemy.health = 10;
	enemy.max_health = 0;
	enemy.damage = 5;
	enemy.xylarite_drop = 0;

	registry.collisionCircles.emplace(entity).radius = 10.f;

	MovementAnimation& anim = registry.movementAnimations.emplace(entity);
	anim.base_scale = { 25.f, 25.f };

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::ENEMY1,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createXylariteCrab(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 50.f; // Scale based on mesh original size

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 6;
	sprite.curr_row = 0;

	Enemy& enemy = registry.enemies.emplace(entity);
	
	// Base stats for xylarite crab enemy type
	int base_health = 1000;
	int base_damage = 34;
	
	float health_multiplier = level_manager.get_enemy_health_multiplier(level, time_in_level_seconds);
	int final_health = static_cast<int>(base_health * health_multiplier);
	
	float damage_multiplier = level_manager.get_enemy_damage_multiplier(level, time_in_level_seconds);
	int final_damage = static_cast<int>(base_damage * damage_multiplier);
	
	enemy.health = final_health;
	enemy.max_health = final_health;
	enemy.damage = final_damage;
	enemy.xylarite_drop = 10;

	// enemy.death_animation = [](Entity entity, float step_seconds) {
	// 	Sprite& sprite = registry.sprites.get(entity);
		
	// 	if(sprite.curr_row == 0) {
	// 		sprite.curr_row = 1;
	// 		sprite.curr_frame = 0;
	// 		sprite.step_seconds_acc = 0.0f;
	// 	}

	// 	if (sprite.step_seconds_acc > sprite.total_frame) {
	// 		registry.remove_all_components_of(entity);
	// 	}
	// };

	// collision circle decoupled from visuals
	registry.collisionCircles.emplace(entity).radius = 18.f;

	// Constrain slime to screen boundaries
	//registry.constrainedEntities.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::XY_CRAB,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createSlime(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 50.f; // Scale based on mesh original size

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 2;
	sprite.total_frame = 6;
	sprite.curr_row = 0;

	Enemy& enemy = registry.enemies.emplace(entity);
	
	// Base stats for slime enemy type
	int base_health = 74;
	int base_damage = 8;
	
	float health_multiplier = level_manager.get_enemy_health_multiplier(level, time_in_level_seconds);
	int final_health = static_cast<int>(base_health * health_multiplier);
	
	float damage_multiplier = level_manager.get_enemy_damage_multiplier(level, time_in_level_seconds);
	int final_damage = static_cast<int>(base_damage * damage_multiplier);
	
	enemy.health = final_health;
	enemy.max_health = final_health;
	enemy.damage = final_damage;
	enemy.xylarite_drop = level;
	enemy.death_animation = [renderer, motion](Entity entity, float step_seconds) {
		Sprite& sprite = registry.sprites.get(entity);
		
		if(sprite.curr_row == 0) {
			sprite.curr_row = 1;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
		}

		if (sprite.step_seconds_acc > sprite.total_frame) {
			registry.remove_all_components_of(entity);
		}
	};

	// collision circle decoupled from visuals
	registry.collisionCircles.emplace(entity).radius = 18.f;

	// Constrain slime to screen boundaries
	//registry.constrainedEntities.emplace(entity);

	TEXTURE_ASSET_ID texture_id = static_cast<TEXTURE_ASSET_ID>(
		static_cast<int>(TEXTURE_ASSET_ID::SLIME_1) + level - 1
	);

	registry.renderRequests.insert(
		entity,
		{ texture_id,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createEvilPlant(RenderSystem* renderer, vec2 pos, const LevelManager& level_manager, int level, float time_in_level_seconds)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 100.f; // Scale based on mesh original size

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 4;
	sprite.total_frame = 4;
	sprite.curr_row = 0;

	TEXTURE_ASSET_ID idle_texure_id = static_cast<TEXTURE_ASSET_ID>(
		static_cast<int>(TEXTURE_ASSET_ID::PLANT_IDLE_1) + (level - 1) * 4
	);

	TEXTURE_ASSET_ID hurt_texure_id = static_cast<TEXTURE_ASSET_ID>(
		static_cast<int>(TEXTURE_ASSET_ID::PLANT_HURT_1) + (level - 1) * 4
	);

	TEXTURE_ASSET_ID death_texure_id = static_cast<TEXTURE_ASSET_ID>(
		static_cast<int>(TEXTURE_ASSET_ID::PLANT_DEATH_1) + (level - 1) * 4
	);

	Enemy& enemy = registry.enemies.emplace(entity);
	
	// Base stats for evil plant enemy type (stronger, stationary)
	int base_health = 150;
	int base_damage = 15;
	
	float health_multiplier = level_manager.get_enemy_health_multiplier(level, time_in_level_seconds);
	int final_health = static_cast<int>(base_health * health_multiplier);
	
	float damage_multiplier = level_manager.get_enemy_damage_multiplier(level, time_in_level_seconds);
	int final_damage = static_cast<int>(base_damage * damage_multiplier);
	
	enemy.health = final_health;
	enemy.max_health = final_health;
	enemy.damage = final_damage;
	enemy.xylarite_drop = level;
	enemy.death_animation = [death_texure_id](Entity entity, float step_seconds) {
		RenderRequest& render = registry.renderRequests.get(entity);
		Sprite& sprite = registry.sprites.get(entity);

		if (render.used_texture != death_texure_id) {
			render.used_texture = death_texure_id;
			sprite.total_row = 4;
			sprite.total_frame = 10;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
		}

		if (sprite.step_seconds_acc > sprite.total_frame - 1) {
			registry.remove_all_components_of(entity);
		}
	};

	enemy.hurt_animation = [hurt_texure_id, idle_texure_id](Entity entity, float step_seconds) {
		RenderRequest& render = registry.renderRequests.get(entity);
		Sprite& sprite = registry.sprites.get(entity);
		Enemy& enemy = registry.enemies.get(entity);

		if (render.used_texture != hurt_texure_id) {
			render.used_texture = hurt_texure_id;
			sprite.total_row = 4;
			sprite.total_frame = 5;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			sprite.animation_speed = 25.f;
		}

		if (sprite.step_seconds_acc > sprite.total_frame - 1) {
			enemy.is_hurt = false;
		}

		if (!enemy.is_hurt) {
			render.used_texture = idle_texure_id;
			sprite.total_row = 4;
			sprite.total_frame = 4;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
			sprite.animation_speed = 10.f;
		}
	};

	StationaryEnemy& stat_enemy = registry.stationaryEnemies.emplace(entity);
	stat_enemy.position = pos;
	
	// Mark slime as an occluder for shadow casting
	// registry.occluders.emplace(entity);
	// Constrain slime to screen boundaries
	// registry.constrainedEntities.emplace(entity);
	
	// collision circle decoupled from visuals
	registry.collisionCircles.emplace(entity).radius = 18.f;

	registry.renderRequests.insert(
		entity,
		{ idle_texure_id, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createBullet(RenderSystem* renderer, vec2 pos, vec2 velocity, int damage)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::BULLET_CIRCLE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = velocity;
	motion.scale = mesh.original_size * 20.f;

	// bullet component with weapon damage
	Bullet& bullet = registry.bullets.emplace(entity);
	bullet.damage = damage;

	// Make bullets emit light
	registry.lights.emplace(entity);
	Light& light = registry.lights.get(entity);
	light.is_enabled = true;
	light.light_color = { 1.0f, 0.8f, 0.3f };
	light.brightness = 1.0f;
	light.range = 100.0f;

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::COLOURED,
			GEOMETRY_BUFFER_ID::BULLET_CIRCLE });
	return entity;
}

Entity createExplosionEffect(RenderSystem* renderer, vec2 pos, float radius)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SPRITE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	float sprite_scale = radius > 0.f ? radius * 1.2f : 90.0f;
	if (sprite_scale < 90.0f) {
		sprite_scale = 90.0f;
	}
	motion.scale = mesh.original_size * sprite_scale;

	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 12;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;
	sprite.animation_speed = 40.0f;

	registry.nonColliders.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::EXPLOSION,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	DeathTimer& timer = registry.deathTimers.emplace(entity);
	timer.counter_ms = 300.0f;

	return entity;
}

Entity createFlashlight(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 1.0f, 1.0f }; 

	registry.lights.emplace(entity);
	Light& light = registry.lights.get(entity);
	light.is_enabled = true;
	light.cone_angle = 0.5f; 
	light.brightness = 2.0f;  
	light.falloff = 0.5f;   
	light.range = 900.0f;
	light.light_color = { 0.6f, 0.75f, 1.0f };
	light.inner_cone_angle = 0.0f; 
	light.offset = { 50.0f, 25.0f };
	light.use_target_angle = true;

	return entity;
}

// sample function to show that you can make lights that can also be associated with different entities
// here, it is an enemy light
Entity createEnemyLight(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 200.0f, 200.0f }; 

	registry.lights.emplace(entity);
	Light& light = registry.lights.get(entity);
	light.is_enabled = true;
	light.cone_angle = 2.0f * M_PI; 
	light.brightness = 0.8f;  
	light.falloff = 0.5f;   
	light.range = 200.0f;     
	light.light_color = { 1.0f, 0.0f, 0.0f };
	light.inner_cone_angle = 0.0f; 
	light.offset = { 0.0f, 0.0f };
	light.use_target_angle = false;

	return entity;
}

Entity createBackground(RenderSystem* renderer)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::BACKGROUND_QUAD);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = { 0.f, 0.f };
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	// Make background extremely large to prevent edges from showing
	// Use a truly massive scale to ensure it always covers the viewport
	motion.scale = { 100000.0f, 100000.0f };

	// Add sprite component for textured rendering (required for TEXTURED effect)
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_row = 1;
	sprite.total_frame = 1;
	sprite.curr_row = 0;
	sprite.curr_frame = 0;
	sprite.should_flip = false;

	registry.nonColliders.emplace(entity);

	registry.colors.insert(entity, vec3(0.4f, 0.4f, 0.4f));

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::GRASS,
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::BACKGROUND_QUAD });

	return entity;
}

inline CHUNK_CELL_STATE iso_bitmap_to_state(unsigned char bitmap) {
	switch (bitmap) {
		case 1: return CHUNK_CELL_STATE::ISO_01;
		case 2: return CHUNK_CELL_STATE::ISO_02;
		case 3: return CHUNK_CELL_STATE::ISO_03;
		case 4: return CHUNK_CELL_STATE::ISO_04;
		case 5: return CHUNK_CELL_STATE::ISO_05;
		case 6: return CHUNK_CELL_STATE::ISO_06;
		case 7: return CHUNK_CELL_STATE::ISO_07;
		case 8: return CHUNK_CELL_STATE::ISO_08;
		case 9: return CHUNK_CELL_STATE::ISO_09;
		case 10: return CHUNK_CELL_STATE::ISO_10;
		case 11: return CHUNK_CELL_STATE::ISO_11;
		case 12: return CHUNK_CELL_STATE::ISO_12;
		case 13: return CHUNK_CELL_STATE::ISO_13;
		case 14: return CHUNK_CELL_STATE::ISO_14;
		case 15: return CHUNK_CELL_STATE::ISO_15;
		default: return CHUNK_CELL_STATE::EMPTY;
	}
}

inline unsigned char state_to_iso_bitmap(CHUNK_CELL_STATE state) {
	switch (state) {
		case CHUNK_CELL_STATE::ISO_01: return 1;
		case CHUNK_CELL_STATE::ISO_02: return 2;
		case CHUNK_CELL_STATE::ISO_03: return 3;
		case CHUNK_CELL_STATE::ISO_04: return 4;
		case CHUNK_CELL_STATE::ISO_05: return 5;
		case CHUNK_CELL_STATE::ISO_06: return 6;
		case CHUNK_CELL_STATE::ISO_07: return 7;
		case CHUNK_CELL_STATE::ISO_08: return 8;
		case CHUNK_CELL_STATE::ISO_09: return 9;
		case CHUNK_CELL_STATE::ISO_10: return 10;
		case CHUNK_CELL_STATE::ISO_11: return 11;
		case CHUNK_CELL_STATE::ISO_12: return 12;
		case CHUNK_CELL_STATE::ISO_13: return 13;
		case CHUNK_CELL_STATE::ISO_14: return 14;
		case CHUNK_CELL_STATE::ISO_15: return 15;
		default: return 0;
	}
}

// create collision circles for an isoline
std::vector<Entity> createIsolineCollisionCircles(vec2 pos, CHUNK_CELL_STATE iso_state) {
	std::vector<Entity> created_entities;

	const unsigned char state_mask = static_cast<unsigned char>(iso_state);
	if (state_mask == 0 || state_mask > 15)
		return created_entities;

	const float block_size = (float)(CHUNK_CELL_SIZE * CHUNK_ISOLINE_SIZE);
	const float isoline_half_size = block_size * 0.5f;
	const float quadrant_offset = block_size * 0.25f;
	const float quadrant_radius = quadrant_offset * M_SQRT_2 * 1.05f; // small bias to cover visuals
	const float center_radius = quadrant_offset * M_SQRT_2;

	std::vector<MultiCircleCollider::Circle> circle_data;
	circle_data.reserve(5);

	auto add_circle = [&](vec2 offset, float radius) {
		MultiCircleCollider::Circle circle;
		circle.offset = offset;
		circle.radius = radius;
		circle_data.push_back(circle);
	};

	int filled_quadrants = 0;
	if (state_mask & 1) { // top-left
		add_circle({ -quadrant_offset, -quadrant_offset }, quadrant_radius);
		filled_quadrants++;
	}
	if (state_mask & 2) { // top-right
		add_circle({ quadrant_offset, -quadrant_offset }, quadrant_radius);
		filled_quadrants++;
	}
	if (state_mask & 4) { // bottom-right
		add_circle({ quadrant_offset, quadrant_offset }, quadrant_radius);
		filled_quadrants++;
	}
	if (state_mask & 8) { // bottom-left
		add_circle({ -quadrant_offset, quadrant_offset }, quadrant_radius);
		filled_quadrants++;
	}

	// Large rocks (3+ quadrants) have a solid center in the texture.
	if (filled_quadrants >= 3) {
		add_circle({ 0.f, 0.f }, center_radius);
	}

	if (circle_data.empty())
		return created_entities;

	Entity isoline_entity = Entity();

	Motion& motion = registry.motions.emplace(isoline_entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 1.f, 1.f };

	MultiCircleCollider& multi = registry.multiCircleColliders.emplace(isoline_entity);
	multi.circles = std::move(circle_data);

	IsolineBoundingBox& bbox = registry.isolineBoundingBoxes.emplace(isoline_entity);
	bbox.center = pos;
	bbox.half_width = isoline_half_size;
	bbox.half_height = isoline_half_size;

	registry.obstacles.emplace(isoline_entity);
	created_entities.push_back(isoline_entity);

	return created_entities;
}

// remove collision circles for an isoline
void removeIsolineCollisionCircles(std::vector<Entity>& collision_entities) {
	for (Entity e : collision_entities) {
		registry.remove_all_components_of(e);
	}
	collision_entities.clear();
}

std::vector<Entity> createIsolineObstacle(RenderSystem* renderer, vec2 pos, CHUNK_CELL_STATE iso_state) {
	return createIsolineCollisionCircles(pos, iso_state);
}

bool is_obstacle(CHUNK_CELL_STATE state) {
	switch (state) {
		case CHUNK_CELL_STATE::EMPTY:
		case CHUNK_CELL_STATE::NO_OBSTACLE_AREA:
			return false;
		default:
			return true;
	}
}

// check if a (potentially unloaded) chunk cell contains a non-isoline obstacle
bool cell_has_obstacle(vec2 chunk_pos, vec2 cell_pos) {
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunks.has(chunk_pos_x, chunk_pos_y)) {
		Chunk& chunk = registry.chunks.get(chunk_pos_x, chunk_pos_y);
		return is_obstacle(chunk.cell_states[(size_t) cell_pos.x][(size_t) cell_pos.y]);
	} else if (registry.serial_chunks.has(chunk_pos_x, chunk_pos_y)) {
		float cell_size = (float) CHUNK_CELL_SIZE;
		float chunk_size = cell_size * (float) CHUNK_CELLS_PER_ROW;

		SerializedChunk& serial_chunk = registry.serial_chunks.get(chunk_pos_x, chunk_pos_y);
		for (SerializedTree serial_tree : serial_chunk.serial_trees) {
			float t_min_x = serial_tree.position.x - (abs(serial_tree.scale) / 2);
			float t_max_x = serial_tree.position.x + (abs(serial_tree.scale) / 2);
			float t_min_y = serial_tree.position.y - (abs(serial_tree.scale) / 2);
			float t_max_y = serial_tree.position.y + (abs(serial_tree.scale) / 2);

			if (chunk_pos.x*chunk_size + cell_size*(cell_pos.x+1) > t_min_x &&
				chunk_pos.x*chunk_size + cell_size*(cell_pos.x) < t_max_x &&
				chunk_pos.y*chunk_size + cell_size*(cell_pos.y+1) > t_min_y &&
				chunk_pos.y*chunk_size + cell_size*(cell_pos.y) < t_max_y)
			{
				return true;
			}
		}
		for (SerializedWall serial_wall : serial_chunk.serial_walls) {
			float w_min_x = serial_wall.position.x - (abs(serial_wall.scale.x) / 2);
			float w_max_x = serial_wall.position.x + (abs(serial_wall.scale.x) / 2);
			float w_min_y = serial_wall.position.y - (abs(serial_wall.scale.y) / 2);
			float w_max_y = serial_wall.position.y + (abs(serial_wall.scale.y) / 2);

			if (chunk_pos.x*chunk_size + cell_size*(cell_pos.x+1) > w_min_x &&
				chunk_pos.x*chunk_size + cell_size*(cell_pos.x) < w_max_x &&
				chunk_pos.y*chunk_size + cell_size*(cell_pos.y+1) > w_min_y &&
				chunk_pos.y*chunk_size + cell_size*(cell_pos.y) < w_max_y)
			{
				return true;
			}
		}
		return false;
	} else {
		// chunk not generated, treat as having no obstacles
		return false;
	}
}

ChunkBoundary& get_chunk_bound(vec2 chunk_pos) {
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunk_bounds.has(chunk_pos_x, chunk_pos_y)) {
		return registry.chunk_bounds.get(chunk_pos_x, chunk_pos_y);
	} else {
		return registry.chunk_bounds.emplace(chunk_pos_x, chunk_pos_y);
	}
}

// Generate a section of the world
Chunk& generateChunk(RenderSystem* renderer, vec2 chunk_pos, PerlinNoiseGenerator& map_noise, PerlinNoiseGenerator& decorator_noise, std::default_random_engine& rng, bool is_spawn_chunk, bool is_boss_chunk) {
	/////////////////////////
	// INITIALIZATION STEP //
	/////////////////////////

	// Check if chunk has already been generated
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunks.has(chunk_pos_x, chunk_pos_y))
		return registry.chunks.get(chunk_pos_x, chunk_pos_y);

	std::uniform_real_distribution<float> uniform_dist;
	float cell_size = (float) CHUNK_CELL_SIZE;
	float cells_per_row = (float) CHUNK_CELLS_PER_ROW;
	float chunk_width = cells_per_row * cell_size;
	float chunk_height = cells_per_row * cell_size;

	vec2 base_world_pos = vec2(chunk_width*((float) chunk_pos_x), chunk_height*((float) chunk_pos_y));
	float noise_scale = (float) CHUNK_NOISE_PER_CHUNK / chunk_width;

	// initialize new chunk
	Chunk& chunk = registry.chunks.emplace(chunk_pos_x, chunk_pos_y);
	chunk.cell_states.resize(CHUNK_CELLS_PER_ROW);
	for (int i = 0; i < CHUNK_CELLS_PER_ROW; i++) {
		chunk.cell_states[i].assign(CHUNK_CELLS_PER_ROW, CHUNK_CELL_STATE::EMPTY);
	}

	// populate chunk cell data
	for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i += CHUNK_ISOLINE_SIZE) {
		for (int u = 0; u < CHUNK_ISOLINE_SIZE; u++) {
			chunk.cell_states[i+u].resize(CHUNK_CELLS_PER_ROW);
		}
	}

	//////////////////
	// ISOLINE STEP //
	//////////////////

	// Compute marching quad (isoline) obstacle data over 4x4 regions of the chunk
	for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i += CHUNK_ISOLINE_SIZE) {
		for (size_t j = 0; j < CHUNK_CELLS_PER_ROW; j += CHUNK_ISOLINE_SIZE) {
			unsigned char iso_quad_state = 0;
			float noise_a = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
								noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
			float noise_b = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
								noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
			float noise_c = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
								noise_scale * (base_world_pos.y + cell_size*((float) j+4.5)));
			float noise_d = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
								noise_scale * (base_world_pos.y + cell_size*((float) j+4.5)));
			
			if (noise_a > CHUNK_ISOLINE_THRESHOLD)
				iso_quad_state += 1;
			if (noise_b > CHUNK_ISOLINE_THRESHOLD)
				iso_quad_state += 2;
			if (noise_c > CHUNK_ISOLINE_THRESHOLD)
				iso_quad_state += 4;
			if (noise_d > CHUNK_ISOLINE_THRESHOLD)
				iso_quad_state += 8;

			// partition cells into "isoline" and "non-isoline" groups
			CHUNK_CELL_STATE state = iso_bitmap_to_state(iso_quad_state);
			
			chunk.cell_states[i][j] = ((iso_quad_state & 1) == 1)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i][j+1] = ((iso_quad_state & 1) == 1)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i][j+2] = ((iso_quad_state & 8) == 8)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i][j+3] = ((iso_quad_state & 8) == 8)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+1][j] = ((iso_quad_state & 1) == 1)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+1][j+1] = ((iso_quad_state & 1) == 1 && (iso_quad_state & 10) > 0)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+1][j+2] = ((iso_quad_state & 8) == 8 && (iso_quad_state & 5) > 0)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+1][j+3] = ((iso_quad_state & 8) == 8)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+2][j] = ((iso_quad_state & 2) == 2)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+2][j+1] = ((iso_quad_state & 2) == 2 && (iso_quad_state & 5) > 0)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+2][j+2] = ((iso_quad_state & 4) == 4 && (iso_quad_state & 10) > 0)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+2][j+3] = ((iso_quad_state & 4) == 4)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+3][j] = ((iso_quad_state & 2) == 2)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+3][j+1] = ((iso_quad_state & 2) == 2)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+3][j+2] = ((iso_quad_state & 4) == 4)
				? state : CHUNK_CELL_STATE::EMPTY;
			chunk.cell_states[i+3][j+3] = ((iso_quad_state & 4) == 4)
				? state : CHUNK_CELL_STATE::EMPTY;

			// find eligible non-isoline cells
			for (int u = 0; u < CHUNK_ISOLINE_SIZE; u++) {
				for (int v = 0; v < CHUNK_ISOLINE_SIZE; v++) {
					if (chunk.cell_states[i+u][j+v] == CHUNK_CELL_STATE::EMPTY) {
						float noise_val = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+u+0.5f)),
							noise_scale * (base_world_pos.y + cell_size*((float) j+v+0.5f)));
						
						if (noise_val < CHUNK_NO_OBSTACLE_THRESHOLD) {
							// mark as empty area
							chunk.cell_states[i+u][j+v] = CHUNK_CELL_STATE::NO_OBSTACLE_AREA;
						}
					}
				}
			}
		} 
	}

	// Filter out isoline data from spawn area
	if (is_spawn_chunk) {
		vec2 spawn_position = {window_width_px/2, window_height_px - 200};
		vec2 local_pos = (spawn_position - base_world_pos) / vec2(cell_size, cell_size);
		int spawn_min_x = (int) (floor(local_pos.x / 4) - 2) * 4;
		int spawn_max_x = (int) (floor(local_pos.x / 4) + 2) * 4;
		int spawn_min_y = (int) (floor(local_pos.y / 4) - 2) * 4;
		int spawn_max_y = (int) (floor(local_pos.y / 4) + 2) * 4;

		// create isoline filters for spawn area
		for (int i = spawn_min_x; i <= spawn_max_x; i += CHUNK_ISOLINE_SIZE) {
			for (int j = spawn_min_y; j <= spawn_max_y; j += CHUNK_ISOLINE_SIZE) {
				if (i < 0 || i >= CHUNK_CELLS_PER_ROW || j < 0 || j >= CHUNK_CELLS_PER_ROW)
					continue;

				IsolineFilter iso_filter;
				iso_filter.upper_left_cell = vec2(i, j);
				iso_filter.lower_right_cell = vec2(i + 3, j + 3);
				iso_filter.reconstruct_upper = (j == spawn_min_y);
				iso_filter.reconstruct_lower = (j == spawn_max_y);
				iso_filter.reconstruct_left = (i == spawn_min_x);
				iso_filter.reconstruct_right = (i == spawn_max_x);
				chunk.iso_filters.push_back(iso_filter);
			}
		}
	}

	// Place structures + add relevant isoline filters
	if (registry.serial_chunks.has(chunk_pos_x, chunk_pos_y)) {
		SerializedChunk& serial_chunk = registry.serial_chunks.get(chunk_pos_x, chunk_pos_y);

		// Remove isolines from structure-reserved areas
		for (IsolineFilter iso_filter : serial_chunk.iso_filters) {
			chunk.iso_filters.push_back(iso_filter);
		}

		// Re-use previous generated wall positions
		for (SerializedWall serial_wall : serial_chunk.serial_walls) {
			// Create obstacle + store in chunk
			Entity wall = createWall(renderer, serial_wall.position, serial_wall.scale);
			chunk.walls.push_back(wall);
		}
	} else {
		// Check if a structure should be generated in this chunk
		float structure_check_noise = uniform_dist(rng); // decorator_noise.noise(chunk_pos_x + 0.1, chunk_pos_y + 0.1);
		printf("structure check noise for chunk (%zi, %zi): %f\n", chunk_pos_x, chunk_pos_y, structure_check_noise);
		if (false) {
			// Generate boss structure across 2 chunks
			float x_scale = 19;
			float y_scale = 11;

			// TODO: determine if this is feasible (and necessary)
			// Find neighbouring, fully ungenerated chunks
			/*
			std::vector<bool> ungen_chunks(9, false);
			for (short i = -1; i <= 1; i++) {
				for (short j = -1; j <= 1; j++) {
					if (i == 0 && j == 0)
						continue;

					if (!registry.chunks.has(chunk_pos_x + i, chunk_pos_y + j)
						&& !registry.serial_chunks.has(chunk_pos_x + i, chunk_pos_y + j))
					{
						ungen_chunks[(i*3) + j + 4] = true;
					}
				}
			}
			bool ul_okay = ungen_chunks[0] && ungen_chunks[1] && ungen_chunks[3];
			bool ur_okay = ungen_chunks[3] && ungen_chunks[6] && ungen_chunks[7];
			bool ll_okay = ungen_chunks[1] && ungen_chunks[2] && ungen_chunks[5];
			bool lr_okay = ungen_chunks[5] && ungen_chunks[7] && ungen_chunks[8];
			*/

			// TODO: add serialized structure data for neighbouring chunks
		} else if (is_boss_chunk || (!is_spawn_chunk && structure_check_noise > CHUNK_STRUCTURE_THRESHOLD)) {
			// Generate generic structure
			float x_scale = 7 + floor(uniform_dist(rng) * 8);
			float y_scale = 7 + floor(uniform_dist(rng) * 8);
			if (x_scale > 14)
				x_scale = 14;
			if (y_scale > 14)
				y_scale = 14;

			float x_shift = 1 + floor(uniform_dist(rng) * (15 - x_scale));
			float y_shift = 1 + floor(uniform_dist(rng) * (15 - y_scale));
			if (x_shift + x_scale > 15)
				x_shift = 15 - x_scale;
			if (y_shift + y_scale > 15)
				y_shift = 15 - y_scale;

			IsolineFilter structure_body;
			structure_body.upper_left_cell = vec2(x_shift * 4, y_shift * 4);
			structure_body.lower_right_cell = vec2((x_shift + x_scale) * 4 - 1, (y_shift + y_scale) * 4 - 1);
			structure_body.reconstruct_upper = false;
			structure_body.reconstruct_lower = false;
			structure_body.reconstruct_left = false;
			structure_body.reconstruct_right = false;
			chunk.iso_filters.push_back(structure_body);

			unsigned short entrance_layout = (short) (1 + floor(uniform_dist(rng) * 14));
			if (entrance_layout > 14)
				entrance_layout = 14;

			if ((entrance_layout & 1) == 1) {
				// generate top entrance
				float cut = 1 + floor(uniform_dist(rng) * (x_scale - 4));
				if (cut > x_scale - 4)
					cut = x_scale - 4;
				
				Entity wall1 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + (cut + 0.5)*CHUNK_ISOLINE_SIZE/2),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + 1)),
					vec2(cell_size*CHUNK_ISOLINE_SIZE*(cut + 0.5), cell_size*2));
				chunk.walls.push_back(wall1);
				Entity wall2 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + (x_scale + cut + 2.5)*CHUNK_ISOLINE_SIZE/2),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + 1)),
					vec2(cell_size*CHUNK_ISOLINE_SIZE*(x_scale - cut - 2.5), cell_size*2));
				chunk.walls.push_back(wall2);

				// clear out isolines in front of entrance
				IsolineFilter filter1;
				filter1.upper_left_cell = vec2((x_shift + cut)*4, (y_shift - 1)*4);
				filter1.lower_right_cell = vec2((x_shift + cut + 1)*4 - 1, y_shift*4 - 1);
				filter1.reconstruct_upper = true;
				filter1.reconstruct_lower = false;
				filter1.reconstruct_left = true;
				filter1.reconstruct_right = false;
				chunk.iso_filters.push_back(filter1);

				IsolineFilter filter2;
				filter2.upper_left_cell = vec2((x_shift + cut + 1)*4, (y_shift - 1)*4);
				filter2.lower_right_cell = vec2((x_shift + cut + 2)*4 - 1, y_shift*4 - 1);
				filter2.reconstruct_upper = true;
				filter2.reconstruct_lower = false;
				filter2.reconstruct_left = false;
				filter2.reconstruct_right = false;
				chunk.iso_filters.push_back(filter2);

				IsolineFilter filter3;
				filter3.upper_left_cell = vec2((x_shift + cut + 2)*4, (y_shift - 1)*4);
				filter3.lower_right_cell = vec2((x_shift + cut + 3)*4 - 1, y_shift*4 - 1);
				filter3.reconstruct_upper = true;
				filter3.reconstruct_lower = false;
				filter3.reconstruct_left = false;
				filter3.reconstruct_right = true;
				chunk.iso_filters.push_back(filter3);
			} else {
				// generate top wall
				Entity wall = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + x_scale*CHUNK_ISOLINE_SIZE/2),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + 1)),
					vec2(cell_size*CHUNK_ISOLINE_SIZE*x_scale, cell_size*2));
				chunk.walls.push_back(wall);
			}

			if ((entrance_layout & 2) == 2) {
				// generate right entrance
				float cut = 1 + floor(uniform_dist(rng) * (y_scale - 4));
				if (cut > y_scale - 4)
					cut = y_scale - 4;
				
				Entity wall1 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.lower_right_cell.x),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + (cut + 0.5)*CHUNK_ISOLINE_SIZE/2)),
					vec2(cell_size*2, cell_size*CHUNK_ISOLINE_SIZE*(cut + 0.5)));
				chunk.walls.push_back(wall1);
				Entity wall2 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.lower_right_cell.x),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + (y_scale + cut + 2.5)*CHUNK_ISOLINE_SIZE/2)),
					vec2(cell_size*2, cell_size*CHUNK_ISOLINE_SIZE*(y_scale - cut - 2.5)));
				chunk.walls.push_back(wall2);

				// clear out isolines in front of entrance
				IsolineFilter filter1;
				filter1.upper_left_cell = vec2((x_shift + x_scale)*4, (y_shift + cut)*4);
				filter1.lower_right_cell = vec2((x_shift + x_scale + 1)*4 - 1, (y_shift + cut + 1)*4 - 1);
				filter1.reconstruct_upper = true;
				filter1.reconstruct_lower = false;
				filter1.reconstruct_left = false;
				filter1.reconstruct_right = true;
				chunk.iso_filters.push_back(filter1);

				IsolineFilter filter2;
				filter2.upper_left_cell = vec2((x_shift + x_scale)*4, (y_shift + cut + 1)*4);
				filter2.lower_right_cell = vec2((x_shift + x_scale + 1)*4 - 1, (y_shift + cut + 2)*4 - 1);
				filter2.reconstruct_upper = false;
				filter2.reconstruct_lower = false;
				filter2.reconstruct_left = false;
				filter2.reconstruct_right = true;
				chunk.iso_filters.push_back(filter2);

				IsolineFilter filter3;
				filter3.upper_left_cell = vec2((x_shift + x_scale)*4, (y_shift + cut + 2)*4);
				filter3.lower_right_cell = vec2((x_shift + x_scale + 1)*4 - 1, (y_shift + cut + 3)*4 - 1);
				filter3.reconstruct_upper = false;
				filter3.reconstruct_lower = true;
				filter3.reconstruct_left = false;
				filter3.reconstruct_right = true;
				chunk.iso_filters.push_back(filter3);
			} else {
				// generate right wall
				Entity wall = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.lower_right_cell.x),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + y_scale*CHUNK_ISOLINE_SIZE/2)),
					vec2(cell_size*2, cell_size*CHUNK_ISOLINE_SIZE*(y_scale - 1)));
				chunk.walls.push_back(wall);
			}

			if ((entrance_layout & 4) == 4) {
				// generate bottom entrance
				float cut = 1 + floor(uniform_dist(rng) * (y_scale - 4));
				if (cut > y_scale - 4)
					cut = y_scale - 4;
				
				Entity wall1 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + (cut + 0.5)*CHUNK_ISOLINE_SIZE/2),
						base_world_pos.y + cell_size*(structure_body.lower_right_cell.y)),
					vec2(cell_size*CHUNK_ISOLINE_SIZE*(cut + 0.5), cell_size*2));
				chunk.walls.push_back(wall1);
				Entity wall2 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + (x_scale + cut + 2.5)*CHUNK_ISOLINE_SIZE/2),
						base_world_pos.y + cell_size*(structure_body.lower_right_cell.y)),
					vec2(cell_size*CHUNK_ISOLINE_SIZE*(x_scale - cut - 2.5), cell_size*2));
				chunk.walls.push_back(wall2);

				// clear out isolines in front of entrance
				IsolineFilter filter1;
				filter1.upper_left_cell = vec2((x_shift + cut)*4, (y_shift + y_scale)*4);
				filter1.lower_right_cell = vec2((x_shift + cut + 1)*4 - 1, (y_shift + y_scale + 1)*4 - 1);
				filter1.reconstruct_upper = false;
				filter1.reconstruct_lower = true;
				filter1.reconstruct_left = true;
				filter1.reconstruct_right = false;
				chunk.iso_filters.push_back(filter1);

				IsolineFilter filter2;
				filter2.upper_left_cell = vec2((x_shift + cut + 1)*4, (y_shift + y_scale)*4);
				filter2.lower_right_cell = vec2((x_shift + cut + 2)*4 - 1, (y_shift + y_scale + 1)*4 - 1);
				filter2.reconstruct_upper = false;
				filter2.reconstruct_lower = true;
				filter2.reconstruct_left = false;
				filter2.reconstruct_right = false;
				chunk.iso_filters.push_back(filter2);

				IsolineFilter filter3;
				filter3.upper_left_cell = vec2((x_shift + cut + 2)*4, (y_shift + y_scale)*4);
				filter3.lower_right_cell = vec2((x_shift + cut + 3)*4 - 1, (y_shift + y_scale + 1)*4 - 1);
				filter3.reconstruct_upper = false;
				filter3.reconstruct_lower = true;
				filter3.reconstruct_left = false;
				filter3.reconstruct_right = true;
				chunk.iso_filters.push_back(filter3);
			} else {
				// generate bottom wall
				Entity wall = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + x_scale*CHUNK_ISOLINE_SIZE/2),
						base_world_pos.y + cell_size*(structure_body.lower_right_cell.y)),
					vec2(cell_size*CHUNK_ISOLINE_SIZE*x_scale, cell_size*2));
				chunk.walls.push_back(wall);
			}

			if ((entrance_layout & 8) == 8) {
				// generate left entrance
				float cut = 1 + floor(uniform_dist(rng) * (x_scale - 4));
				if (cut > x_scale - 4)
					cut = x_scale - 4;

				Entity wall1 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + 1),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + (cut + 0.5)*CHUNK_ISOLINE_SIZE/2)),
					vec2(cell_size*2, cell_size*CHUNK_ISOLINE_SIZE*(cut + 0.5)));
				chunk.walls.push_back(wall1);
				Entity wall2 = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + 1),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + (y_scale + cut + 2.5)*CHUNK_ISOLINE_SIZE/2)),
					vec2(cell_size*2, cell_size*CHUNK_ISOLINE_SIZE*(y_scale - cut - 3.5)));
				chunk.walls.push_back(wall2);

				// clear out isolines in front of entrance
				IsolineFilter filter1;
				filter1.upper_left_cell = vec2((x_shift - 1)*4, (y_shift + cut)*4);
				filter1.lower_right_cell = vec2(x_shift*4 - 1, (y_shift + cut + 1)*4 - 1);
				filter1.reconstruct_upper = true;
				filter1.reconstruct_lower = false;
				filter1.reconstruct_left = true;
				filter1.reconstruct_right = false;
				chunk.iso_filters.push_back(filter1);

				IsolineFilter filter2;
				filter2.upper_left_cell = vec2((x_shift - 1)*4, (y_shift + cut + 1)*4);
				filter2.lower_right_cell = vec2(x_shift*4 - 1, (y_shift + cut + 2)*4 - 1);
				filter2.reconstruct_upper = false;
				filter2.reconstruct_lower = false;
				filter2.reconstruct_left = true;
				filter2.reconstruct_right = false;
				chunk.iso_filters.push_back(filter2);

				IsolineFilter filter3;
				filter3.upper_left_cell = vec2((x_shift - 1)*4, (y_shift + cut + 2)*4);
				filter3.lower_right_cell = vec2(x_shift*4 - 1, (y_shift + cut + 3)*4 - 1);
				filter3.reconstruct_upper = false;
				filter3.reconstruct_lower = true;
				filter3.reconstruct_left = true;
				filter3.reconstruct_right = false;
				chunk.iso_filters.push_back(filter3);
			} else {
				// generate left wall
				Entity wall = createWall(renderer,
					vec2(base_world_pos.x + cell_size*(structure_body.upper_left_cell.x + 1),
						base_world_pos.y + cell_size*(structure_body.upper_left_cell.y + y_scale*CHUNK_ISOLINE_SIZE/2)),
					vec2(cell_size*2, cell_size*CHUNK_ISOLINE_SIZE*(y_scale - 1)));
				chunk.walls.push_back(wall);
			}
		}
	}

	// Clean up incomplete isolines
	for (const IsolineFilter& iso_filter : chunk.iso_filters) {
		// set all cells to blank
		size_t x_min = (size_t) max(iso_filter.upper_left_cell.x, 0.0f);
		size_t y_min = (size_t) max(iso_filter.upper_left_cell.y, 0.0f);
		size_t x_max = (size_t) min(iso_filter.lower_right_cell.x, cells_per_row - 1.0f);
		size_t y_max = (size_t) min(iso_filter.lower_right_cell.y, cells_per_row - 1.0f);
		for (size_t x = x_min; x <= x_max; x++) {
			for (size_t y = y_min; y <= y_max; y++) {
				chunk.cell_states[x][y] = CHUNK_CELL_STATE::NO_OBSTACLE_AREA;
			}
		}

		// Perform isoline reconstruction (if needed)
		if (iso_filter.reconstruct_upper || iso_filter.reconstruct_lower || iso_filter.reconstruct_left || iso_filter.reconstruct_right) {
			int iso_x_min = (int) floor(iso_filter.upper_left_cell.x / 4) * 4;
			int iso_y_min = (int) floor(iso_filter.upper_left_cell.y / 4) * 4;
			int iso_x_max = (int) floor(iso_filter.lower_right_cell.x / 4) * 4;
			int iso_y_max = (int) floor(iso_filter.lower_right_cell.y / 4) * 4;
			for (int i = iso_x_min; i <= iso_x_max; i += CHUNK_ISOLINE_SIZE) {
				for (size_t j = iso_y_min; j <= iso_y_max; j += CHUNK_ISOLINE_SIZE) {
					if (i < 0 || i >= CHUNK_CELLS_PER_ROW || j < 0 || j >= CHUNK_CELLS_PER_ROW)
						continue;

					size_t zi = (size_t) i;
					size_t zj = (size_t) j;

					unsigned char iso_quad_state = 0;
					float noise_a = 0;
					float noise_b = 0;
					float noise_c = 0;
					float noise_d = 0;

					// conditionally generate noise
					if (iso_filter.reconstruct_upper || iso_filter.reconstruct_left) {
						noise_a = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
					}
					if (iso_filter.reconstruct_upper || iso_filter.reconstruct_right) {
						noise_b = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
					}
					if (iso_filter.reconstruct_lower || iso_filter.reconstruct_right) {
						noise_c = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+4.5)));
					}
					if (iso_filter.reconstruct_lower || iso_filter.reconstruct_left) {
						noise_d = map_noise.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+4.5)));
					}

					if (noise_a > CHUNK_ISOLINE_THRESHOLD)
						iso_quad_state += 1;
					if (noise_b > CHUNK_ISOLINE_THRESHOLD)
						iso_quad_state += 2;
					if (noise_c > CHUNK_ISOLINE_THRESHOLD)
						iso_quad_state += 4;
					if (noise_d > CHUNK_ISOLINE_THRESHOLD)
						iso_quad_state += 8;

					CHUNK_CELL_STATE state = iso_bitmap_to_state(iso_quad_state);

					if ((iso_quad_state & 1) == 1) {
						chunk.cell_states[zi][zj] = state;
						chunk.cell_states[zi][zj+1] = state;
						chunk.cell_states[zi+1][zj] = state;

						if ((iso_quad_state & 10) > 0)
							chunk.cell_states[zi+1][zj+1] = state;
					}
					if ((iso_quad_state & 2) == 2) {
						chunk.cell_states[zi+2][zj] = state;
						chunk.cell_states[zi+3][zj] = state;
						chunk.cell_states[zi+3][zj+1] = state;

						if ((iso_quad_state & 5) > 0)
							chunk.cell_states[zi+2][zj+1] = state;
					}
					if ((iso_quad_state & 4) == 4) {
						chunk.cell_states[zi+2][zj+3] = state;
						chunk.cell_states[zi+3][zj+2] = state;
						chunk.cell_states[zi+3][zj+3] = state;

						if ((iso_quad_state & 10) > 0)
							chunk.cell_states[zi+2][zj+2] = state;
					}
					if ((iso_quad_state & 8) == 8) {
						chunk.cell_states[zi][zj+2] = state;
						chunk.cell_states[zi][zj+3] = state;
						chunk.cell_states[zi+1][zj+3] = state;

						if ((iso_quad_state & 5) > 0)
							chunk.cell_states[zi+1][zj+2] = state;
					}
				}
			}
		}
	}

	// Generate isoline collision data
	for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i += CHUNK_ISOLINE_SIZE) {
		for (size_t j = 0; j < CHUNK_CELLS_PER_ROW; j += CHUNK_ISOLINE_SIZE) {
			unsigned char s_bit_1 = state_to_iso_bitmap(chunk.cell_states[i][j]);
			unsigned char s_bit_2 = state_to_iso_bitmap(chunk.cell_states[i][j+3]);
			unsigned char s_bit_3 = state_to_iso_bitmap(chunk.cell_states[i+3][j]);
			unsigned char s_bit_4 = state_to_iso_bitmap(chunk.cell_states[i+3][j+3]);
			unsigned char max_bit = max(max(max(s_bit_1, s_bit_2), s_bit_3), s_bit_4);

			if (max_bit != 0) {
				// center of isoline block
				vec2 isoline_pos = base_world_pos + vec2(
					cell_size * ((float) i + (float) CHUNK_ISOLINE_SIZE / 2.0f),
					cell_size * ((float) j + (float) CHUNK_ISOLINE_SIZE / 2.0f)
				);
				IsolineData isoline_data;
				isoline_data.position = isoline_pos;
				isoline_data.state = iso_bitmap_to_state(max_bit);
				isoline_data.collision_entities = std::vector<Entity>();
				chunk.isoline_data.push_back(isoline_data);
			}
		}
	}
	
	////////////////////
	// DECORATOR STEP //
	////////////////////

	// Mark wall cells as obstacles
	for (Entity wall : chunk.walls) {
		Motion& w_motion = registry.motions.get(wall);

		int x_adjust = (int) chunk_pos_x*chunk_width - cell_size/2;
		int y_adjust = (int) chunk_pos_y*chunk_height - cell_size/2;
		int i_min = (int) (w_motion.position.x - abs(w_motion.scale.x/2) - x_adjust) / cell_size;
		int i_max = (int) (w_motion.position.x + abs(w_motion.scale.x/2) - x_adjust) / cell_size;
		int j_min = (int) (w_motion.position.y - abs(w_motion.scale.y/2) - y_adjust) / cell_size;
		int j_max = (int) (w_motion.position.y + abs(w_motion.scale.y/2) - y_adjust) / cell_size;

		for (int i = i_min; i < i_max; i++) {
			for (int j = j_min; j < j_max; j++) {
				if (!is_obstacle(chunk.cell_states[(size_t) i][(size_t) j])) {
					chunk.cell_states[(size_t) i][(size_t) j] = CHUNK_CELL_STATE::OBSTACLE;
				}
			}
		}
	}

	// Mark obstacle cells originating from neighbouring chunks
	if (registry.chunk_bounds.has(chunk_pos_x, chunk_pos_y)) {
		ChunkBoundary& chunk_bound = registry.chunk_bounds.get(chunk_pos_x, chunk_pos_y);
		for (SerializedTree serial_tree : chunk_bound.serial_trees) {
			int cell_coord_x = (serial_tree.position.x - chunk_pos_x*chunk_width - cell_size/2) / cell_size;
			int cell_coord_y = (serial_tree.position.y - chunk_pos_y*chunk_height - cell_size/2) / cell_size;
			int bound = (int) ceil((serial_tree.scale - 16) / 32);

			int i_min = max(cell_coord_x - bound, 0);
			int i_max = min(cell_coord_x + bound, (int) cells_per_row - 1);
			int j_min = max(cell_coord_y - bound, 0);
			int j_max = min(cell_coord_y + bound, (int) cells_per_row - 1);

			for (int i = i_min; i <= i_max; i++) {
				for (int j = j_min; j <= j_max; j++) {
					if (!is_obstacle(chunk.cell_states[(size_t) i][(size_t) j])) {
						chunk.cell_states[(size_t) i][(size_t) j] = CHUNK_CELL_STATE::OBSTACLE;
					}
				}
			}
		}
	}

	// Check if decorator needs to be run
	if (registry.serial_chunks.has(chunk_pos_x, chunk_pos_y) && registry.serial_chunks.get(chunk_pos_x, chunk_pos_y).decorated) {
		// Chunk already generated before: use existing data
		SerializedChunk& serial_chunk = registry.serial_chunks.get(chunk_pos_x, chunk_pos_y);

		// Re-use previously-generated tree positions
		for (SerializedTree serial_tree : serial_chunk.serial_trees) {
			// Create obstacle + store in chunk
			Entity tree = createTree(renderer, serial_tree.position, serial_tree.scale);
			chunk.trees.push_back(tree);

			// Mark relevant cells as obstacles
			int cell_coord_x = (serial_tree.position.x - chunk_pos_x*chunk_width - cell_size/2) / cell_size;
			int cell_coord_y = (serial_tree.position.y - chunk_pos_y*chunk_height - cell_size/2) / cell_size;
			int bound = (int) ceil((serial_tree.scale - 16) / 32);

			int i_min = max(cell_coord_x - bound, 0);
			int i_max = min(cell_coord_x + bound, (int) cells_per_row - 1);
			int j_min = max(cell_coord_y - bound, 0);
			int j_max = min(cell_coord_y + bound, (int) cells_per_row - 1);

			for (int i = i_min; i <= i_max; i++) {
				for (int j = j_min; j <= j_max; j++) {
					if (!is_obstacle(chunk.cell_states[(size_t) i][(size_t) j])) {

						chunk.cell_states[(size_t) i][(size_t) j] = CHUNK_CELL_STATE::OBSTACLE;
					}
				}
			}
		}
	} else {
		// Get eligible cells
		std::vector<vec2> eligible_cells;
		for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i++) {
			for (size_t j = 0; j < CHUNK_CELLS_PER_ROW; j++) {
				if (chunk.cell_states[i][j] == CHUNK_CELL_STATE::EMPTY) {
					eligible_cells.push_back(vec2(i, j));
					//vec2 pushed_vec = eligible_cells[eligible_cells.size() - 1];
				}
			}
		}

		// Run decorator to place trees
		size_t trees_to_place = CHUNK_TREE_DENSITY * eligible_cells.size() / (CHUNK_CELLS_PER_ROW * CHUNK_CELLS_PER_ROW);

		for (size_t i = 0; i < trees_to_place; i++) {
			if (eligible_cells.size() == 0) {
				break;
			}
			int eligibility = 0;
			vec2 selected_cell = eligible_cells[0];

			while (eligibility == 0) {
				if (eligible_cells.size() == 0) {
					eligibility = -1;
					break;
				}

				int max_constraint = CHUNK_TREE_MAX_BOUND + 1;
				size_t n_cell = (size_t) (uniform_dist(rng) * eligible_cells.size());
				if (n_cell == eligible_cells.size())
					n_cell--;
				selected_cell = eligible_cells[n_cell];

				// find obstacles in area around cell
				for (int dx = -CHUNK_TREE_MAX_BOUND; dx <= CHUNK_TREE_MAX_BOUND; dx++) {
					if (abs(dx) < max_constraint) {
						for (int dy = -CHUNK_TREE_MAX_BOUND; dy <= CHUNK_TREE_MAX_BOUND; dy++) {
							if (abs(dy) < max_constraint) {
								if ((selected_cell.x + dx) >= 0 && (selected_cell.x + dx) < CHUNK_CELLS_PER_ROW
									&& (selected_cell.y + dy) >= 0 && (selected_cell.y + dy) < CHUNK_CELLS_PER_ROW)
								{
									// check data inside of current chunk
									if (is_obstacle(chunk.cell_states[(size_t) selected_cell.x+dx][(size_t) selected_cell.y+dy]))
										max_constraint = min(max_constraint, max(abs(dx), abs(dy)));
								} else {
									// check data outside of current chunk
									int shift_x = 0;
									if ((selected_cell.x + dx) < 0)
										shift_x = -1;
									else if ((selected_cell.x + dx) >= CHUNK_CELLS_PER_ROW)
										shift_x = 1;

									int shift_y = 0;
									if ((selected_cell.y + dy) < 0)
										shift_y = -1;
									else if ((selected_cell.y + dy) >= CHUNK_CELLS_PER_ROW)
										shift_y = 1;

									vec2 chunk_to_check = vec2(chunk_pos_x + shift_x, chunk_pos_y + shift_y);
									vec2 cell_to_check = vec2(selected_cell.x + dx - shift_x*cells_per_row,
										selected_cell.y + dy - shift_y*cells_per_row);

									if (cell_has_obstacle(chunk_to_check, cell_to_check))
										max_constraint = min(max_constraint, max(abs(dx), abs(dy)));
								}
							}
						}
					}
				}

				// check final obstacle eligibility
				eligibility = max((max_constraint - 1), 0);
				if (eligibility == 0) {
					// remove cell from eligibility list
					vec2 last = eligible_cells[eligible_cells.size() - 1];
					eligible_cells[n_cell] = last;
					eligible_cells.pop_back();
				}
			}

			// if no more valid positions, stop generating obstacles
			if (eligibility == -1)
				break;

			// Generate obstacle data
			float pos_x = (float) selected_cell.x * cell_size;
			float pos_y = (float) selected_cell.y * cell_size;
			vec2 pos(chunk_pos.x * chunk_width + pos_x + cell_size/2,
					chunk_pos.y * chunk_height + pos_y + cell_size/2);
			
			float scale = 32;
			if (eligibility == 2) {
				float r_val = floor(uniform_dist(rng) * 6);
				if (r_val == 6)
					r_val--;
				scale += 8 * r_val;
			} else {
				float r_val = floor(uniform_dist(rng) * 3);
				if (r_val == 3)
					r_val--;
				scale += 8 * r_val;
			}
			scale += 8;
			
			// Create obstacle + store in chunk
			Entity tree = createTree(renderer, pos, scale);
			chunk.trees.push_back(tree);

			Motion& t_motion = registry.motions.get(tree);
			float t_min_x = t_motion.position.x - (abs(t_motion.scale.x) / 2);
			float t_max_x = t_motion.position.x + (abs(t_motion.scale.x) / 2);
			float t_min_y = t_motion.position.y - (abs(t_motion.scale.y) / 2);
			float t_max_y = t_motion.position.y + (abs(t_motion.scale.y) / 2);

			// remove occupied cells
			for (size_t n = 0; n < eligible_cells.size();) {
				vec2 pair = eligible_cells[n];

				if (base_world_pos.x + cell_size*((float) pair.x+1) <= t_min_x ||
					base_world_pos.x + cell_size*((float) pair.x) >= t_max_x ||
					base_world_pos.y + cell_size*((float) pair.y+1) <= t_min_y ||
					base_world_pos.y + cell_size*((float) pair.y) >= t_max_y)
				{
						n++;
				} else {
					chunk.cell_states[(size_t) pair.x][(size_t) pair.y] = CHUNK_CELL_STATE::OBSTACLE;
					vec2 last = eligible_cells[eligible_cells.size() - 1];
					eligible_cells[n] = last;
					eligible_cells.pop_back();
				}
			}

			// update chunk boundary data
			int bound = (int) ceil((scale - 16) / 32);
			if (selected_cell.x - bound < 0 || selected_cell.x + bound >= CHUNK_CELLS_PER_ROW
				|| selected_cell.y - bound < 0 || selected_cell.y + bound >= CHUNK_CELLS_PER_ROW)
			{
				SerializedTree serial_tree;
				serial_tree.position = pos;
				serial_tree.scale = scale;

				if (selected_cell.x - bound < 0) {
					ChunkBoundary& cb_x = get_chunk_bound(vec2(chunk_pos_x-1, chunk_pos_y));
					cb_x.serial_trees.push_back(serial_tree);

					if (selected_cell.y - bound < 0) {
						ChunkBoundary& cb_y = get_chunk_bound(vec2(chunk_pos_x, chunk_pos_y-1));
						cb_y.serial_trees.push_back(serial_tree);
						ChunkBoundary& cb_xy = get_chunk_bound(vec2(chunk_pos_x-1, chunk_pos_y-1));
						cb_xy.serial_trees.push_back(serial_tree);
					} else if (selected_cell.y + bound >= CHUNK_CELLS_PER_ROW) {
						ChunkBoundary& cb_y = get_chunk_bound(vec2(chunk_pos_x, chunk_pos_y+1));
						cb_y.serial_trees.push_back(serial_tree);
						ChunkBoundary& cb_xy = get_chunk_bound(vec2(chunk_pos_x-1, chunk_pos_y+1));
						cb_xy.serial_trees.push_back(serial_tree);
					}
				} else if (selected_cell.x + bound >= CHUNK_CELLS_PER_ROW) {
					ChunkBoundary& cb_x = get_chunk_bound(vec2(chunk_pos_x+1, chunk_pos_y));
					cb_x.serial_trees.push_back(serial_tree);

					if (selected_cell.y - bound < 0) {
						ChunkBoundary& cb_y = get_chunk_bound(vec2(chunk_pos_x, chunk_pos_y-1));
						cb_y.serial_trees.push_back(serial_tree);
						ChunkBoundary& cb_xy = get_chunk_bound(vec2(chunk_pos_x+1, chunk_pos_y-1));
						cb_xy.serial_trees.push_back(serial_tree);
					} else if (selected_cell.y + bound >= CHUNK_CELLS_PER_ROW) {
						ChunkBoundary& cb_y = get_chunk_bound(vec2(chunk_pos_x, chunk_pos_y+1));
						cb_y.serial_trees.push_back(serial_tree);
						ChunkBoundary& cb_xy = get_chunk_bound(vec2(chunk_pos_x+1, chunk_pos_y+1));
						cb_xy.serial_trees.push_back(serial_tree);
					}
				} else if (selected_cell.y - bound < 0) {
					ChunkBoundary& cb_y = get_chunk_bound(vec2(chunk_pos_x, chunk_pos_y-1));
					cb_y.serial_trees.push_back(serial_tree);
				} else if (selected_cell.y + bound >= CHUNK_CELLS_PER_ROW) {
					ChunkBoundary& cb_y = get_chunk_bound(vec2(chunk_pos_x, chunk_pos_y+1));
					cb_y.serial_trees.push_back(serial_tree);
				}
			}
		}
	}

	return chunk;
}

