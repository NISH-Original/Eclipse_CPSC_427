#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <unordered_map>

class AudioSystem {
private:
    std::unordered_map<std::string, Mix_Chunk*> sounds;
	int master_volume = MIX_MAX_VOLUME;
	bool muted = false;

public:
    bool init();
    bool load(const std::string& name, const std::string& filepath);
    void play(const std::string& name, bool loop = false);
    void stop(const std::string& name);
    void stop_all();
    void cleanup();
	void set_master_volume(int volume);
	int get_master_volume() const { return master_volume; }
	void set_muted(bool value);
	void toggle_muted();
	bool is_muted() const { return muted; }

private:
	void apply_volume();
};
