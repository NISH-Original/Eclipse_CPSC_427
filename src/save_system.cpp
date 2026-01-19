#include "save_system.hpp"
#include "world_system.hpp"

#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

SaveSystem::SaveSystem()
{
}

SaveSystem::~SaveSystem()
{
}

std::string SaveSystem::get_save_directory() const
{
	return data_path() + "/saves";
}

std::string SaveSystem::get_save_filepath(const std::string& save_name) const
{
	return get_save_directory() + "/" + save_name + ".json";
}

bool SaveSystem::ensure_save_directory_exists() const
{
	std::string save_dir = get_save_directory();

	std::ifstream test(save_dir);
	if (!test.good())
	{
		MKDIR(save_dir.c_str());
	}

	return true;
}

bool SaveSystem::has_save_file(const std::string& save_name) const
{
	std::string filepath = get_save_filepath(save_name);
	std::ifstream file(filepath);
	return file.good();
}

void SaveSystem::delete_save(const std::string& save_name)
{
	std::string filepath = get_save_filepath(save_name);
	std::remove(filepath.c_str());
}


bool SaveSystem::save_game(const std::string& save_name)
{
	if (!world_system)
	{
		std::cerr << "Cannot save: WorldSystem not set" << std::endl;
		return false;
	}

	if (!ensure_save_directory_exists())
	{
		std::cerr << "Failed to create save directory" << std::endl;
		return false;
	}

	try
	{
		json save_data = world_system->serialize();

		std::string filepath = get_save_filepath(save_name);
		std::ofstream file(filepath);

		if (!file.is_open())
		{
			std::cerr << "Failed to open save file for writing: " << filepath << std::endl;
			return false;
		}

		file << save_data.dump(4);
		file.close();

		std::cout << "Game saved successfully to: " << filepath << std::endl;
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error saving game: " << e.what() << std::endl;
		return false;
	}
}

bool SaveSystem::load_game(const std::string& save_name)
{
	if (!world_system)
	{
		std::cerr << "Cannot load: WorldSystem not set" << std::endl;
		return false;
	}

	if (!has_save_file(save_name))
	{
		std::cerr << "Save file not found: " << save_name << std::endl;
		return false;
	}

	try
	{
		std::string filepath = get_save_filepath(save_name);
		std::ifstream file(filepath);

		if (!file.is_open())
		{
			std::cerr << "Failed to open save file for reading: " << filepath << std::endl;
			return false;
		}

		json save_data = json::parse(file);
		file.close();

		world_system->deserialize(save_data);

		std::cout << "Game loaded successfully from: " << filepath << std::endl;
		return true;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error loading game: " << e.what() << std::endl;
		return false;
	}
}
