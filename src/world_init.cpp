#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

Entity createPlayer(RenderSystem* renderer, vec2 pos)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::PLAYER_CIRCLE);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = mesh.original_size * 50.f; // Scale based on mesh original size

	// create an empty Salmon component for our character
	registry.players.emplace(entity);
	registry.constrainedEntities.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::SALMON,
			GEOMETRY_BUFFER_ID::PLAYER_CIRCLE });

	return entity;
}

Entity createTree(RenderSystem* renderer, vec2 pos, vec2 scale)
{
	auto entity = Entity();

	// Store a reference to the potentially re-used mesh object
	Mesh& mesh = renderer->getMesh(GEOMETRY_BUFFER_ID::TEXTURED);
	registry.meshPtrs.emplace(entity, &mesh);

	// Setting initial motion values
	Motion& motion = registry.motions.emplace(entity);
	motion.position = pos;
	motion.angle = 0.f;
	motion.velocity = { 0.f, 0.f };
	motion.scale = { mesh.original_size.x * scale.x, mesh.original_size.y * scale.y }; // Scale based on mesh original size

	// create component for our tree
	registry.obstacles.emplace(entity);
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE});
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
	
	// Mark enemy as an occluder for shadow casting
	registry.occluders.emplace(entity);
	
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::LIGHT,
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
	
	// Mark slime as an occluder for shadow casting
	registry.occluders.emplace(entity);
	
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

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TEXTURE_COUNT,
			EFFECT_ASSET_ID::SALMON,
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
	motion.scale = { 200.0f, 200.0f }; 

	registry.lights.emplace(entity);
	Light& light = registry.lights.get(entity);
	light.is_enabled = true;
	light.cone_angle = 0.5f; 
	light.brightness = 0.8f;  
	light.falloff = 0.5f;   
	light.range = 900.0f;     
	light.light_color = { 1.0f, 1.0f, 1.0f };
	light.inner_cone_angle = 0.0f; 
	light.offset = { 40.0f, 0.0f };
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
			EFFECT_ASSET_ID::LIGHT,
			GEOMETRY_BUFFER_ID::BACKGROUND_QUAD });

	return entity;
}
