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
	sprite.total_row = 1;
	sprite.total_frame = 1;

	registry.obstacles.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::TREE, // TEXTURE_COUNT indicates that no texture is needed
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

	// collision circle decoupled from visuals
	registry.collisionCircles.emplace(entity).radius = 18.f;

	// Constrain slime to screen boundaries
	//registry.constrainedEntities.emplace(entity);

	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PLANT_IDLE, // TEXTURE_COUNT indicates that no texture is needed
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

	// Mark slime as an occluder for shadow casting
	// registry.occluders.emplace(entity);
	// Constrain slime to screen boundaries
	registry.constrainedEntities.emplace(entity);
	
	registry.renderRequests.insert(
		entity,
		{ TEXTURE_ASSET_ID::PLANT_IDLE, // TEXTURE_COUNT indicates that no texture is needed
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

// Generate a section of the world
Chunk& generate_chunk(RenderSystem* renderer, vec2 chunk_pos, PerlinNoiseGenerator noise_func, std::default_random_engine rng) {
	// check if chunk has already been generated
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunks.has(chunk_pos_x, chunk_pos_y))
		return registry.chunks.get(chunk_pos_x, chunk_pos_y);

	float cell_size = (float) CHUNK_CELL_SIZE;
	float cells_per_row = (float) CHUNK_CELLS_PER_ROW;
	float chunk_width = cells_per_row * cell_size;
	float chunk_height = cells_per_row * cell_size;

	vec2 base_world_pos = vec2(chunk_width*((float) chunk_pos_x), chunk_height*((float) chunk_pos_y));
	float noise_scale = (float) CHUNK_NOISE_PER_CHUNK / chunk_width;

	float p_min_x, p_max_x, p_min_y, p_max_y;
	if (registry.players.size() == 0) {
		p_min_x = 0;
		p_max_x = 0;
		p_min_y = 0;
		p_max_y = 0;
	} else {
		Entity player = registry.players.entities[0];
		Motion& p_motion = registry.motions.get(player);
		
		p_min_x = p_motion.position.x - (abs(p_motion.scale.x) / 2);
		p_max_x = p_motion.position.x + (abs(p_motion.scale.x) / 2);
		p_min_y = p_motion.position.y - (abs(p_motion.scale.y) / 2);
		p_max_y = p_motion.position.y + (abs(p_motion.scale.y) / 2);
	}

	// initialize new chunk
	Chunk& chunk = registry.chunks.emplace(chunk_pos_x, chunk_pos_y);
	chunk.cell_states.resize(CHUNK_CELLS_PER_ROW);

	// populate chunk cell data + generate list of eligible positions
	// TODO: remove positions with entities handing over chunk borders
	std::vector<vec2> eligible_cells;
	for (size_t i = 0; i < CHUNK_CELLS_PER_ROW; i++) {
		chunk.cell_states[i].resize(CHUNK_CELLS_PER_ROW);
		for (size_t j = 0; j < CHUNK_CELLS_PER_ROW; j++) {
			if (noise_func.noise((base_world_pos.x + cell_size*((float) i+0.5))*noise_scale,
								 (base_world_pos.y + cell_size*((float) j+0.5))*noise_scale) < 0) {
				chunk.cell_states[i][j] = CHUNK_CELL_STATE::NO_OBSTACLE_AREA;
			} else {
				chunk.cell_states[i][j] = CHUNK_CELL_STATE::EMPTY;
				if (base_world_pos.x + cell_size*((float) i+2) <= p_min_x ||
					base_world_pos.x + cell_size*((float) i-1) >= p_max_x ||
					base_world_pos.y + cell_size*((float) j+2) <= p_min_y ||
					base_world_pos.y + cell_size*((float) j-1) >= p_max_y)
				{
					eligible_cells.push_back(vec2(i, j));
				}
			}
		}
	}

	// Check if decorator needs to be run
	if (registry.serial_chunks.has(chunk_pos_x, chunk_pos_y)) {
		// Chunk already generated before: re-use previously-generated tree positions
		SerializedChunk& serial_chunk = registry.serial_chunks.get(chunk_pos_x, chunk_pos_y);
		for (SerializedTree serial_tree : serial_chunk.serial_trees) {
			// Create obstacle + store in chunk
			Entity tree = createTree(renderer, serial_tree.position);
			chunk.persistent_entities.push_back(tree);

			// Mark relevant cells as obstacles
			vec2 cell_coord = (serial_tree.position - vec2(cell_size/2, cell_size/2) - vec2(chunk_pos_x, chunk_pos_y)) / cell_size;
			for (size_t i = cell_coord.x - 1; i <= cell_coord.x + 1; i++) {
				for (size_t j = cell_coord.y - 1; i <= cell_coord.y + 1; i++) {
					if (i >= 0 && j >= 0 && i < cells_per_row && j < cells_per_row)
						chunk.cell_states[i][j] = CHUNK_CELL_STATE::OBSTACLE;
				}
			}
		}
	} else {
		// Run decorator to place trees
		size_t trees_to_place = CHUNK_TREE_DENSITY * eligible_cells.size() / (CHUNK_CELLS_PER_ROW * CHUNK_CELLS_PER_ROW);
		std::uniform_real_distribution<float> uniform_dist;

		printf("Debug info for generation of chunk (%i, %i):\n", chunk_pos_x, chunk_pos_y);
		printf("   %zi valid cells\n", eligible_cells.size());
		printf("   %zi trees to be placed in chunk\n", trees_to_place);

		for (size_t i = 0; i < trees_to_place; i++) {
			size_t n_cell = (size_t) (uniform_dist(rng) * eligible_cells.size());
			vec2 selected_cell = eligible_cells[n_cell];
			float pos_x = (float) selected_cell.x * cell_size;
			float pos_y = (float) selected_cell.y * cell_size;
			vec2 pos(chunk_pos.x * chunk_width + pos_x + cell_size/2,
					chunk_pos.y * chunk_height + pos_y + cell_size/2);
			
			// Create obstacle + store in chunk
			Entity tree = createTree(renderer, pos);
			chunk.persistent_entities.push_back(tree);

			Motion& t_motion = registry.motions.get(tree);
			float t_min_x = t_motion.position.x - (abs(t_motion.scale.x) / 2);
			float t_max_x = t_motion.position.x + (abs(t_motion.scale.x) / 2);
			float t_min_y = t_motion.position.y - (abs(t_motion.scale.y) / 2);
			float t_max_y = t_motion.position.y + (abs(t_motion.scale.y) / 2);

			// remove occupied cells
			for (size_t n = 0; n < eligible_cells.size();) {
				vec2 pair = eligible_cells[n];

				// TODO: add similar check on placement
				if (base_world_pos.x + cell_size*((float) pair.x+1) <= t_min_x ||
					base_world_pos.x + cell_size*((float) pair.x) >= t_max_x ||
					base_world_pos.y + cell_size*((float) pair.y+1) <= t_min_y ||
					base_world_pos.y + cell_size*((float) pair.y) >= t_max_y)
				{
					float i_diff = abs(pair.x - selected_cell.x);
					float j_diff = abs(pair.y - selected_cell.y);
					if (i_diff <= 3 && j_diff <= 3) {
						vec2 last = eligible_cells[eligible_cells.size() - 1];
						eligible_cells[n] = last;
						eligible_cells.pop_back();
					} else {
						n++;
					}
				} else {
					chunk.cell_states[(size_t) pair.x][(size_t) pair.y] = CHUNK_CELL_STATE::OBSTACLE;
					vec2 last = eligible_cells[eligible_cells.size() - 1];
					eligible_cells[n] = last;
					eligible_cells.pop_back();
				}
			}
		}
	}

	return chunk;
}
