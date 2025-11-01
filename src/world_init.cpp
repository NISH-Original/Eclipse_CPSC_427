#include "world_init.hpp"
#include "tiny_ecs_registry.hpp"

// Tree density configuration
// - 2048*2048 / 1280*720 ≈ 4.55 scale factor
// - 20 (original) maps to 91 (scaled)
const size_t CHUNK_CELL_SIZE = 16;
const size_t CHUNK_CELLS_PER_ROW = 128;
const size_t CHUNK_CELLS_PER_COLUMN = 128;
const int TREE_DENSITY = 60;

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

	// create an empty Salmon component for our character
	registry.players.emplace(entity);
	// Constrain player to screen boundaries
	registry.constrainedEntities.emplace(entity);

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
	registry.occluders.emplace(entity);
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
	
	// Mark enemy as an occluder for shadow casting
	registry.occluders.emplace(entity);
	// Constrain enemy to screen boundaries
	registry.constrainedEntities.emplace(entity);

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

// Generate a section of the world
Chunk& generate_chunk(vec2 chunk_pos, PerlinNoiseGenerator noise) {
	// check if chunk has already been generated
	short chunk_pos_x = (short) chunk_pos.x;
	short chunk_pos_y = (short) chunk_pos.y;
	if (registry.chunks.has(chunk_pos_x, chunk_pos_y))
		return registry.chunks.get(chunk_pos_x, chunk_pos_y);

	float cell_size = (float) CHUNK_CELL_SIZE;
	float cells_per_row = (float) CHUNK_CELLS_PER_ROW;
	float cells_per_col = (float) CHUNK_CELLS_PER_COLUMN;

	Entity player = registry.players.entities[0];
	Motion& p_motion = registry.motions.get(player);
	float p_min_x = p_motion.position.x - (abs(p_motion.scale.x) / 2);
	float p_max_x = p_motion.position.x + (abs(p_motion.scale.x) / 2);
	float p_min_y = p_motion.position.y - (abs(p_motion.scale.y) / 2);
	float p_max_y = p_motion.position.y + (abs(p_motion.scale.y) / 2);

	// initialize new chunk
	Chunk& chunk = registry.chunks.emplace(chunk_pos_x, chunk_pos_y);
	chunk.cell_states.resize(CHUNK_CELLS_PER_ROW);

	// generate list of eligible positions
	std::vector<std::pair<float, float>> eligible_cells;
	for (float i = 1; i < cells_per_row - 1; i++) {
		for (float j = 1; j < cells_per_col - 1; j++) {
			if ((i+2)*cell_size <= p_min_x || (i-1)*cell_size >= p_max_x ||
				(j+2)*cell_size <= p_min_y || (j-1)*cell_size >= p_max_y) {
				eligible_cells.push_back(std::pair<float, float>(i, j));
			}
		}
	}
	printf("Debug info for generation of chunk (%i, %i):\n", chunk_pos_x, chunk_pos_y);
	printf("   %i valid cells\n", eligible_cells.size());
	printf("   Player x min/max: %f and %f\n", p_min_x, p_max_x);
	printf("   Player y min/max: %f and %f\n", p_min_y, p_max_y);

	// place trees
    size_t trees_to_place = TREE_DENSITY * eligible_cells.size() / (CHUNK_CELLS_PER_ROW * CHUNK_CELLS_PER_COLUMN);

	for (size_t i = 0; i < trees_to_place; i++) {
		/*size_t n_cell = (size_t) (uniform_dist(rng) * eligible_cells.size());
		std::pair<float, float> selected_cell = eligible_cells[n_cell];
		float pos_x = (selected_cell.first + uniform_dist(rng)) * cell_size;
		float pos_y = (selected_cell.second + uniform_dist(rng)) * cell_size;
		vec2 pos(chunk_pos.x * cells_per_row * cell_size + pos_x,
			     chunk_pos.y * cells_per_col * cell_size + pos_y);
		
		Entity tree = createTree(renderer, pos);
		chunk.persistent_entities.push_back(tree);

		// remove ineligible cells
		for (size_t n = 0; n < eligible_cells.size();) {
			std::pair<float, float> pair = eligible_cells[n];
			float i_diff = abs(pair.first - selected_cell.first);
			float j_diff = abs(pair.second - selected_cell.second);
			if (i_diff <= 2 && j_diff <= 2) {
				std::pair<float, float> last = eligible_cells[eligible_cells.size() - 1];
				eligible_cells[n] = last;
				eligible_cells.pop_back();
			} else {
				n++;
			}
		}*/
	}

	return chunk;
}
