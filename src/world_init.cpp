#include "common.hpp"
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
		sprite.total_row = 1;
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
	registry.collisionCircles.emplace(entity).radius = 100.f;

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
	//registry.constrainedEntities.emplace(entity);

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
	sprite.total_row = 2;
	sprite.total_frame = 6;
	sprite.curr_row = 0;

	Enemy& enemy = registry.enemies.emplace(entity);
	enemy.death_animation = [](Entity entity, float step_seconds) {
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

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::SLIME, // TEXTURE_COUNT indicates that no texture is needed
			EFFECT_ASSET_ID::TEXTURED,
			GEOMETRY_BUFFER_ID::SPRITE });

	return entity;
}

Entity createEvilPlant(RenderSystem* renderer, vec2 pos)
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

	Enemy& enemy = registry.enemies.emplace(entity);
	enemy.death_animation = [](Entity entity, float step_seconds) {
		RenderRequest& render = registry.renderRequests.get(entity);
		Sprite& sprite = registry.sprites.get(entity);

		if (render.used_texture != TEXTURE_ASSET_ID::PLANT_DEATH) {
			render.used_texture = TEXTURE_ASSET_ID::PLANT_DEATH;
			sprite.total_row = 4;
			sprite.total_frame = 10;
			sprite.curr_frame = 0;
			sprite.step_seconds_acc = 0.0f;
		}

		if (sprite.step_seconds_acc > sprite.total_frame - 1) {
			registry.remove_all_components_of(entity);
		}
	};

	enemy.hurt_animation = [](Entity entity, float step_seconds) {
		RenderRequest& render = registry.renderRequests.get(entity);
		Sprite& sprite = registry.sprites.get(entity);
		Enemy& enemy = registry.enemies.get(entity);

		if (render.used_texture != TEXTURE_ASSET_ID::PLANT_HURT) {
			render.used_texture = TEXTURE_ASSET_ID::PLANT_HURT;
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
			render.used_texture = TEXTURE_ASSET_ID::PLANT_IDLE;
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
		{ TEXTURE_ASSET_ID::PLANT_IDLE, // TEXTURE_COUNT indicates that no texture is needed
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

CHUNK_CELL_STATE iso_bitmap_to_state(unsigned char bitmap) {
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
Chunk& generateChunk(RenderSystem* renderer, vec2 chunk_pos, PerlinNoiseGenerator noise_func, std::default_random_engine rng, bool is_spawn_chunk) {
	// check if chunk has already been generated
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunks.has(chunk_pos_x, chunk_pos_y))
		return registry.chunks.get(chunk_pos_x, chunk_pos_y);

	printf("Generating chunk (%i, %i)...\n", chunk_pos_x, chunk_pos_y);

	float cell_size = (float) CHUNK_CELL_SIZE;
	float cells_per_row = (float) CHUNK_CELLS_PER_ROW;
	float chunk_width = cells_per_row * cell_size;
	float chunk_height = cells_per_row * cell_size;

	vec2 base_world_pos = vec2(chunk_width*((float) chunk_pos_x), chunk_height*((float) chunk_pos_y));
	float noise_scale = (float) CHUNK_NOISE_PER_CHUNK / chunk_width;

	int p_min_x, p_max_x, p_min_y, p_max_y;
	if (is_spawn_chunk && registry.players.size() > 0) {
		Entity player = registry.players.entities[0];
		Motion& p_motion = registry.motions.get(player);

		vec2 local_pos = (p_motion.position - base_world_pos) / vec2(cell_size, cell_size);
		p_min_x = (int) (floor(local_pos.x / 4) - 2) * 4;
		p_max_x = (int) (floor(local_pos.x / 4) + 2) * 4;
		p_min_y = (int) (floor(local_pos.y / 4) - 2) * 4;
		p_max_y = (int) (floor(local_pos.y / 4) + 2) * 4;
	} else {
		p_min_x = 0;
		p_max_x = 0;
		p_min_y = 0;
		p_max_y = 0;
	}

	// initialize new chunk
	Chunk& chunk = registry.chunks.emplace(chunk_pos_x, chunk_pos_y);
	chunk.cell_states.resize(CHUNK_CELLS_PER_ROW);
	for (int i = 0; i < CHUNK_CELLS_PER_ROW; i++) {
		chunk.cell_states[i].assign(CHUNK_CELLS_PER_ROW, CHUNK_CELL_STATE::EMPTY);
	}

	// populate chunk cell data + generate list of eligible positions
	// TODO: ensure player is not trapped inside an obstacle on spawn
	std::vector<vec2> eligible_cells;

	// mark obstacle cells originating from neighbouring chunks
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

	for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i += CHUNK_ISOLINE_SIZE) {
		for (size_t j = 0; j < CHUNK_CELLS_PER_ROW; j += CHUNK_ISOLINE_SIZE) {
			if (!is_spawn_chunk || i < p_min_x || i > p_max_x || j < p_min_y || j > p_max_y) {
				// not in player's "safe" area: compute isoline data for isoline block
				unsigned char iso_quad_state = 0;
				float noise_a = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
				float noise_b = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
				float noise_c = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
									noise_scale * (base_world_pos.y + cell_size*((float) j+4.5)));
				float noise_d = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
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

				if ((iso_quad_state & 1) == 1) {
					chunk.cell_states[i][j] = state;
					chunk.cell_states[i][j+1] = state;
					chunk.cell_states[i+1][j] = state;

					if ((iso_quad_state & 10) > 0)
						chunk.cell_states[i+1][j+1] = state;
				}
				if ((iso_quad_state & 2) == 2) {
					chunk.cell_states[i+2][j] = state;
					chunk.cell_states[i+3][j] = state;
					chunk.cell_states[i+3][j+1] = state;

					if ((iso_quad_state & 5) > 0)
						chunk.cell_states[i+2][j+1] = state;
				}
				if ((iso_quad_state & 4) == 4) {
					chunk.cell_states[i+2][j+3] = state;
					chunk.cell_states[i+3][j+2] = state;
					chunk.cell_states[i+3][j+3] = state;

					if ((iso_quad_state & 10) > 0)
						chunk.cell_states[i+2][j+2] = state;
				}
				if ((iso_quad_state & 8) == 8) {
					chunk.cell_states[i][j+2] = state;
					chunk.cell_states[i][j+3] = state;
					chunk.cell_states[i+1][j+3] = state;

					if ((iso_quad_state & 5) > 0)
						chunk.cell_states[i+1][j+2] = state;
				}

				// find eligible non-isoline cells
				for (int u = 0; u < CHUNK_ISOLINE_SIZE; u++) {
					for (int v = 0; v < CHUNK_ISOLINE_SIZE; v++) {
						if (chunk.cell_states[i+u][j+v] == CHUNK_CELL_STATE::EMPTY) {
							float noise_val = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+u+0.5f)),
								noise_scale * (base_world_pos.y + cell_size*((float) j+v+0.5f)));
							
							if (noise_val < CHUNK_NO_OBSTACLE_THRESHOLD) {
								// mark as empty area
								chunk.cell_states[i+u][j+v] = CHUNK_CELL_STATE::NO_OBSTACLE_AREA;
							} else {
								// mark as eligible cell for obstacle placement
								eligible_cells.push_back(vec2(i+u, j+v));
								vec2 pushed_vec = eligible_cells[eligible_cells.size() - 1];
							}
						}
					}
				}
			} else {
				// player area: do not generate obstacles here
				for (int u = 0; u < CHUNK_ISOLINE_SIZE; u++) {
					for (int v = 0; v < CHUNK_ISOLINE_SIZE; v++) {
						chunk.cell_states[i+u][j+v] = CHUNK_CELL_STATE::NO_OBSTACLE_AREA;
					}
				}
			}
		}
	}

	// Clean up incomplete isolines
	if (p_min_x < 0)
		p_min_x = 0;
	if (p_max_x > CHUNK_CELLS_PER_ROW - CHUNK_ISOLINE_SIZE)
		p_max_x = CHUNK_CELLS_PER_ROW - CHUNK_ISOLINE_SIZE;
	if (p_min_y < 0)
		p_min_y = 0;
	if (p_max_y > CHUNK_CELLS_PER_ROW - CHUNK_ISOLINE_SIZE)
		p_max_y = CHUNK_CELLS_PER_ROW - CHUNK_ISOLINE_SIZE;

	for (int i = p_min_x; i <= p_max_x; i += CHUNK_ISOLINE_SIZE) {
		for (int j = p_min_y; j <= p_max_y; j += CHUNK_ISOLINE_SIZE) {
			size_t zi = (size_t) i;
			size_t zj = (size_t) j;

			unsigned char iso_quad_state = 0;
			float noise_a = 0;
			float noise_b = 0;
			float noise_c = 0;
			float noise_d = 0;

			// conditionally generate noise
			if (i == p_min_x || j == p_min_y) {
				noise_a = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
							noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
			}
			if (i == p_max_x || j == p_min_y) {
				noise_b = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
							noise_scale * (base_world_pos.y + cell_size*((float) j+0.5)));
			}
			if (i == p_max_x || j == p_max_y) {
				noise_c = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+4.5)),
							noise_scale * (base_world_pos.y + cell_size*((float) j+4.5)));
			}
			if (i == p_min_x || j == p_max_y) {
				noise_d = noise_func.noise(noise_scale * (base_world_pos.x + cell_size*((float) i+0.5)),
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

	// Check if decorator needs to be run
	if (registry.serial_chunks.has(chunk_pos_x, chunk_pos_y)) {
		// Chunk already generated before: re-use previously-generated tree positions
		SerializedChunk& serial_chunk = registry.serial_chunks.get(chunk_pos_x, chunk_pos_y);
		for (SerializedTree serial_tree : serial_chunk.serial_trees) {
			// Create obstacle + store in chunk
			Entity tree = createTree(renderer, serial_tree.position, serial_tree.scale);
			chunk.persistent_entities.push_back(tree);

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
		// Run decorator to place trees
		size_t trees_to_place = CHUNK_TREE_DENSITY * eligible_cells.size() / (CHUNK_CELLS_PER_ROW * CHUNK_CELLS_PER_ROW);
		std::uniform_real_distribution<float> uniform_dist;

		printf("Debug info for decoration of chunk (%i, %i):\n", chunk_pos_x, chunk_pos_y);
		printf("   %zi valid cells\n", eligible_cells.size());
		printf("   %zi trees to be placed in chunk\n", trees_to_place);

		for (size_t i = 0; i < trees_to_place; i++) {
			if (eligible_cells.size() == 0) {
				printf("No more eligible cells: %zi out of %zi trees placed\n", i, trees_to_place);
				break;
			}
			int eligibility = 0;
			vec2 selected_cell = eligible_cells[0];

			while (eligibility == 0) {
				if (eligible_cells.size() == 0) {
					eligibility = -1;
					printf("No more eligible cells: %zi out of %zi trees placed\n", i, trees_to_place);
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
			chunk.persistent_entities.push_back(tree);

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

	printf("Finished generating chunk (%i, %i)\n", chunk_pos_x, chunk_pos_y);
	return chunk;
}
