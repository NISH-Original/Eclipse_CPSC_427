#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <set>
#include <functional>
#include <typeindex>
#include <assert.h>

// Unique identifyer for all entities
class Entity
{
	unsigned int id;
	static unsigned int id_count; // starts from 1, entit 0 is the default initialization
public:
	Entity()
	{
		id = id_count++;
		// Note, indices of already deleted entities arent re-used in this simple implementation.
	}
	operator unsigned int() { return id; } // this enables automatic casting to int
};

// Common interface to refer to all containers in the ECS registry
struct ContainerInterfaceBase
{
	virtual void clear() = 0;
	virtual size_t size() = 0;
};

// Common interface to refer to all entity-based containers in the ECS registry
struct ContainerInterface : public ContainerInterfaceBase
{
	virtual void remove(Entity e) = 0;
	virtual bool has(Entity entity) = 0;
};

// Common interface to refer to all position-based containers in the ECS registry
struct PositionalContainerInterface : public ContainerInterfaceBase
{
	virtual void remove(short x, short y) = 0;
	virtual bool has(short x, short y) = 0;
};

// A container that stores components of type 'Component' and associated entities
template <typename Component> // A component can be any class
class ComponentContainer : public ContainerInterface
{
private:
	// The hash map from Entity -> array index.
	std::unordered_map<unsigned int, unsigned int> map_entity_componentID; // the entity is cast to uint to be hashable.
	bool registered = false;
public:
	// Container of all components of type 'Component'
	std::vector<Component> components;

	// The corresponding entities
	std::vector<Entity> entities;

	// Constructor that registers the type
	ComponentContainer()
	{
	}

	// Inserting a component c associated to entity e
	inline Component& insert(Entity e, Component c, bool check_for_duplicates = true)
	{
		// Usually, every entity should only have one instance of each component type
		assert(!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry");

		map_entity_componentID[e] = (unsigned int)components.size();
		components.push_back(std::move(c)); // the move enforces move instead of copy constructor
		entities.push_back(e);
		return components.back();
	};

	// The emplace function takes the the provided arguments Args, creates a new object of type Component, and inserts it into the ECS system
	template<typename... Args>
	Component& emplace(Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...));
	};
	template<typename... Args>
	Component& emplace_with_duplicates(Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...), false);
	};

	// A wrapper to return the component of an entity
	Component& get(Entity e) {
		assert(has(e) && "Entity not contained in ECS registry");
		return components[map_entity_componentID[e]];
	}

	// Check if entity has a component of type 'Component'
	bool has(Entity entity) {
		return map_entity_componentID.count(entity) > 0;
	}

	// Remove an component and pack the container to re-use the empty space
	void remove(Entity e)
	{
		if (has(e))
		{
			// Get the current position
			int cID = map_entity_componentID[e];

			// Move the last element to position cID using the move operator
			// Note, components[cID] = components.back() would trigger the copy instead of move operator
			components[cID] = std::move(components.back());
			entities[cID] = entities.back(); // the entity is only a single index, copy it.
			map_entity_componentID[entities.back()] = cID;

			// Erase the old component and free its memory
			map_entity_componentID.erase(e);
			components.pop_back();
			entities.pop_back();
			// Note, one could mark the id for re-use
		}
	};

	// Remove all components of type 'Component'
	void clear()
	{
		map_entity_componentID.clear();
		components.clear();
		entities.clear();
	}

	// Report the number of components of type 'Component'
	size_t size()
	{
		return components.size();
	}

	// Sort the components and associated entity assignment structures by the comparisonFunction, see std::sort
	template <class Compare>
	void sort(Compare comparisonFunction)
	{
		// First sort the entity list as desired
		std::sort(entities.begin(), entities.end(), comparisonFunction);
		// Now re-arrange the components (Note, creates a new vector, which may be slow! Not sure if in-place could be faster: https://stackoverflow.com/questions/63703637/how-to-efficiently-permute-an-array-in-place-using-stdswap)
		std::vector<Component> components_new; components_new.reserve(components.size());
		std::transform(entities.begin(), entities.end(), std::back_inserter(components_new), [&](Entity e) { return std::move(get(e)); }); // note, the get still uses the old hash map (on purpose!)
		components = std::move(components_new); // note, we use move operations to not create unneccesary copies of objects, but memory is still allocated for the new vector
		// Fill the new hashmap
		for (unsigned int i = 0; i < entities.size(); i++)
			map_entity_componentID[entities[i]] = i;
	}
};

// A container that stores components of type 'Component' and associated world positions
// Used for components that are properties of world positions rather than entities
template <typename Component> // A component can be any class
class PositionalComponentContainer : public PositionalContainerInterface
{
private:
	// The hash map from position -> array index.
	std::unordered_map<int, unsigned int> map_pos_componentID;

	bool registered = false;

	int posKey(short x, short y) {
		int shift_y = ((int) y) << 16;
		return (int) x + shift_y;
	}
public:
	// Container of all components of type 'Component'
	std::vector<Component> components;

	// The corresponding positions
	std::vector<short> position_xs;
	std::vector<short> position_ys;

	// Constructor that registers the type
	PositionalComponentContainer()
	{
	}

	// Inserting a component c associated to position (x, y)
	inline Component& insert(short x, short y, Component c, bool check_for_duplicates = true)
	{
		assert(!(check_for_duplicates && has(x, y)) && "Entity already contained in ECS registry");

		int key = posKey(x, y);
		map_pos_componentID[key] = (unsigned int) components.size();

		components.push_back(std::move(c)); // the move enforces move instead of copy constructor
		position_xs.push_back(x);
		position_ys.push_back(y);

		return components.back();
	};

	// The emplace function takes the the provided arguments Args, creates a new object of type Component, and inserts it into the ECS system
	template<typename... Args>
	Component& emplace(short x, short y, Args &&... args) {
		return insert(x, y, Component(std::forward<Args>(args)...));
	};
	template<typename... Args>
	Component& emplace_with_duplicates(short x, short y, Args &&... args) {
		return insert(x, y, Component(std::forward<Args>(args)...), false);
	};

	// A wrapper to return the component of an entity at a given position
	Component& get(short x, short y) {
		assert(has(x, y) && "Entity not contained in ECS registry");
		int key = posKey(x, y);
		return components[map_pos_componentID[key]];
	}

	// Check if position has a component of type 'Component'
	bool has(short x, short y) {
		int key = posKey(x, y);
		return map_pos_componentID.count(key) > 0;
	}

	// Remove an component and pack the container to re-use the empty space
	void remove(short x, short y)
	{
		if (has(x, y))
		{
			// Get the current position
			int key = posKey(x, y);
			int cID = map_pos_componentID[key];

			// Move the last element to position cID using the move operator
			// Note, components[cID] = components.back() would trigger the copy instead of move operator
			components[cID] = std::move(components.back());
			position_xs[cID] = position_xs.back();
			position_ys[cID] = position_ys.back();
			int newKey = posKey(position_xs[cID], position_ys[cID]);
			map_pos_componentID[newKey] = cID;

			// Erase the old component and free its memory
			map_pos_componentID.erase(key);
			components.pop_back();
			position_xs.pop_back();
			position_ys.pop_back();
		}
	};

	// Remove all components of type 'Component'
	void clear()
	{
		map_pos_componentID.clear();
		components.clear();
		position_xs.clear();
		position_ys.clear();
	}

	// Report the number of components of type 'Component'
	size_t size()
	{
		return components.size();
	}

	// Sort the components and associated positions assignment structures by the comparisonFunction, see std::sort
	/*template <class Compare>
	void sort(Compare comparisonFunction)
	{
		// First sort the position list as desired
		std::sort(entities.begin(), entities.end(), comparisonFunction);
		// Now re-arrange the components (Note, creates a new vector, which may be slow! Not sure if in-place could be faster: https://stackoverflow.com/questions/63703637/how-to-efficiently-permute-an-array-in-place-using-stdswap)
		std::vector<Component> components_new; components_new.reserve(components.size());
		std::transform(entities.begin(), entities.end(), std::back_inserter(components_new), [&](Entity e) { return std::move(get(e)); }); // note, the get still uses the old hash map (on purpose!)
		components = std::move(components_new); // note, we use move operations to not create unneccesary copies of objects, but memory is still allocated for the new vector
		// Fill the new hashmap
		for (unsigned int i = 0; i < entities.size(); i++)
			map_posIndex_componentID[entities[i]] = i;
	}*/
};