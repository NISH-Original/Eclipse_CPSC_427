#pragma once

#include "common.hpp"
#include <json.hpp>
#include <string>

using json = nlohmann::json;

class WorldSystem;

class SaveSystem
{
public:
	SaveSystem();
	~SaveSystem();

	bool save_game(const std::string& save_name = "savegame");
	bool load_game(const std::string& save_name = "savegame");
	bool has_save_file(const std::string& save_name = "savegame") const;
	void delete_save(const std::string& save_name = "savegame");

	void set_world_system(WorldSystem* world) { world_system = world; }

private:
	std::string get_save_directory() const;
	std::string get_save_filepath(const std::string& save_name) const;
	bool ensure_save_directory_exists() const;

	WorldSystem* world_system = nullptr;
};
