#pragma once
#include <vector>

#include "tiny_ecs.hpp"
#include "components.hpp"

class ECSRegistry
{
	// Callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registry_list;
	std::vector<PositionalContainerInterface*> positional_registry_list;

public:
	// Manually created list of all components this game has
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<Motion> motions;
	ComponentContainer<Collision> collisions;
	ComponentContainer<Player> players;
	ComponentContainer<Obstacle> obstacles;
	ComponentContainer<ConstrainedToScreen> constrainedEntities;
	ComponentContainer<Mesh*> meshPtrs;
	ComponentContainer<RenderRequest> renderRequests;
	ComponentContainer<ScreenState> screenStates;
	ComponentContainer<DebugComponent> debugComponents;
	ComponentContainer<vec3> colors;
	ComponentContainer<Light> lights;
	ComponentContainer<Enemy> enemies;
	ComponentContainer<Bullet> bullets;
	ComponentContainer<Sprite> sprites;
	ComponentContainer<CollisionMesh> colliders;
	ComponentContainer<Feet> feet;
	ComponentContainer<Arrow> arrows;
	ComponentContainer<CollisionCircle> collisionCircles;
	ComponentContainer<IsolineBoundingBox> isolineBoundingBoxes;
	ComponentContainer<Weapon> weapons;
	ComponentContainer<Armor> armors;
	ComponentContainer<Inventory> inventories;
	ComponentContainer<DamageCooldown> damageCooldowns;
	ComponentContainer<Steering> enemy_steerings;
	ComponentContainer<AccumulatedForce> enemy_dirs;
	ComponentContainer<Deadly> deadlies;
	ComponentContainer<StationaryEnemy> stationaryEnemies;

	PositionalComponentContainer<Chunk> chunks;
	PositionalComponentContainer<ChunkBoundary> chunk_bounds;
	PositionalComponentContainer<SerializedChunk> serial_chunks;
	

	// constructor that adds all containers for looping over them
	// IMPORTANT: Don't forget to add any newly added containers!
	ECSRegistry()
	{
		registry_list.push_back(&deathTimers);
		registry_list.push_back(&motions);
		registry_list.push_back(&collisions);
		registry_list.push_back(&players);
		registry_list.push_back(&obstacles);
		registry_list.push_back(&constrainedEntities);
		registry_list.push_back(&meshPtrs);
		registry_list.push_back(&renderRequests);
		registry_list.push_back(&screenStates);
		registry_list.push_back(&debugComponents);
		registry_list.push_back(&colors);
		registry_list.push_back(&lights);
		registry_list.push_back(&enemies);
		registry_list.push_back(&bullets);
		registry_list.push_back(&sprites);
		registry_list.push_back(&colliders);
		registry_list.push_back(&feet);
		registry_list.push_back(&arrows);
		registry_list.push_back(&collisionCircles);
		registry_list.push_back(&isolineBoundingBoxes);
		registry_list.push_back(&weapons);
		registry_list.push_back(&armors);
		registry_list.push_back(&inventories);
		registry_list.push_back(&damageCooldowns);
		registry_list.push_back(&enemy_steerings);
		registry_list.push_back(&enemy_dirs);
		registry_list.push_back(&deadlies);
		registry_list.push_back(&stationaryEnemies);

		positional_registry_list.push_back(&chunks);
		positional_registry_list.push_back(&chunk_bounds);
		positional_registry_list.push_back(&serial_chunks);
	}

	void clear_all_components() {
		for (ContainerInterface* reg : registry_list)
			reg->clear();
		for (PositionalContainerInterface* reg : positional_registry_list)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
		for (PositionalContainerInterface* reg : positional_registry_list)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registry_list)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}
	void list_all_components_of(short x, short y) {
		printf("Debug info on components of position (%i, %i):\n", x, y);
		for (PositionalContainerInterface* reg : positional_registry_list)
			if (reg->has(x, y))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(Entity e) {
		for (ContainerInterface* reg : registry_list)
			reg->remove(e);
	}
	void remove_all_components_of(short x, short y) {
		for (PositionalContainerInterface* reg : positional_registry_list)
			reg->remove(x, y);
	}
};

extern ECSRegistry registry;