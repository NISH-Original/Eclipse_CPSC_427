#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity createPlayer(RenderSystem* renderer, vec2 pos)
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

	// Create sprite component for animation
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_frame = sprite.idle_frames; // Start with idle animation
	sprite.current_animation = TEXTURE_ASSET_ID::PLAYER_IDLE;

	// add collision mesh for player (for damage detection only)
	{
		CollisionMesh& col = registry.colliders.emplace(entity);
		col.local_points = {
			{ -0.3f, -0.2f }, { -0.3f,  0.3f }, { -0.2f,  0.35f }, {  0.1f,  0.35f },
			{  0.2f,  0.3f }, {  0.44f,  0.3f }, {  0.44f,  0.2f }, {  0.25f,  0.2f },
			{  0.3f, -0.09f }, {  0.0f, -0.2f }, {  0.0f, -0.3f }
		};
	}

	// add collision circle for player (for blocking/pushing)
	{
		vec2 bb = { abs(motion.scale.x), abs(motion.scale.y) };
		float radius = sqrtf(bb.x*bb.x + bb.y*bb.y) / 5.f;
		registry.collisionCircles.emplace(entity).radius = radius;
	}

	// create an empty Salmon component for our character
	registry.players.emplace(entity);
	// registry.constrainedEntities.emplace(entity);

	// Add a radial light to the player
	Light& player_light = registry.lights.emplace(entity);
	player_light.light_color = vec3(1.0f, 0.9f, 0.7f);
	player_light.follow_target = Entity();
	player_light.offset = vec2(0.0f, 0.0f);
	player_light.range = 500.0f;
	player_light.cone_angle = 3.14159f;
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

	// initial values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 90.f;

    // sprite component for animation
    Sprite& sprite = registry.sprites.emplace(entity);
    sprite.total_frame = 20; // Feet walk has 20 frames
    sprite.current_animation = TEXTURE_ASSET_ID::FEET_WALK;

	// feet component
	Feet& feet = registry.feet.emplace(entity);
	feet.parent_player = parent_player;

    registry.renderRequests.insert(
        entity,
        { TEXTURE_ASSET_ID::FEET_WALK,
            EFFECT_ASSET_ID::TEXTURED,
            GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createTree(RenderSystem* renderer, vec2 pos)
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
	motion.scale = mesh.original_size * 40.f; // Scale based on mesh original size

	// create component for our tree
	Sprite& sprite = registry.sprites.emplace(entity);
	sprite.total_frame = 1;

	registry.obstacles.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TREE, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE});

	return entity;
}

Entity createEnemy(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::ENEMY_TRIANGLE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 50.f; // Scale based on mesh original size

	registry.enemies.emplace(entity);

	// add collision mesh for triangle enemy
	{
		CollisionMesh& col = registry.colliders.emplace(entity);
		col.local_points = {
			{ -0.433f, -0.5f }, { -0.433f,  0.5f }, {  0.433f,  0.0f }
		};
	}

	// Constrain enemy to screen boundaries
	registry.constrainedEntities.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::COLOURED,
			GEOMETRY_BUFFER_ID::ENEMY_TRIANGLE });

	return entity;
}

Entity createSlime(RenderSystem* renderer, vec2 pos)
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
	sprite.total_frame = 6;

	registry.enemies.emplace(entity);

	// collision circle decoupled from visuals
	registry.collisionCircles.emplace(entity).radius = 18.f;

	// Constrain slime to screen boundaries
	registry.constrainedEntities.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::SLIME, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createBullet(RenderSystem* renderer, vec2 pos, vec2 velocity)
{
	auto entity = Entity();

	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::BULLET_CIRCLE);
	registry.meshPtrs.emplace(entity, &mesh);

	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = velocity;
	motion.scale = mesh.original_size * 20.f;

	// Add bullet component
	registry.bullets.emplace(entity);

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
	light.brightness = 0.8f;  
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
	motion.position = { 0, 0 };
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { 2000.0f, 2000.0f };

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::COLOURED,
			GEOMETRY_BUFFER_ID::BACKGROUND_QUAD });

	return entity;
}
