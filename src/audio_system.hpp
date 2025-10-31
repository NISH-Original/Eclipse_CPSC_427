#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <unordered_map>

class AudioSystem {
private:
    std::unordered_map<std::string, Mix_Chunk*> sounds;

public:
    bool init();
    bool load(const std::string& name, const std::string& filepath);
    void play(const std::string& name, bool loop = false);
    void cleanup();
};
