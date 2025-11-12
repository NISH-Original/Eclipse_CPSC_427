#include "audio_system.hpp"
#include <iostream>

bool AudioSystem::init() {
    // Initialize SDL audio if not already initialized
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL Audio: " << SDL_GetError() << std::endl;
        return false;
    }

    // Open audio device
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1) {
        std::cerr << "Failed to open audio device: " << Mix_GetError() << std::endl;
        return false;
    }

    Mix_AllocateChannels(16);

    std::cout << "Audio system initialized successfully" << std::endl;
    return true;
}


// Load a sound file into the audio system
bool AudioSystem::load(const std::string& name, const std::string& filepath) {
    Mix_Chunk* sound = Mix_LoadWAV(filepath.c_str());

    if (!sound) {
        std::cerr << "Failed to load sound: " << filepath << " - " << Mix_GetError() << std::endl;
        return false;
    }

    // Add the sound to the map
    sounds[name] = sound;
    std::cout << "Loaded sound: " << name << " from " << filepath << std::endl;
    return true;
}

// Play a sound, optionally loop it by setting loop to true
void AudioSystem::play(const std::string& name, bool loop) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        // -1 loops forever, 0 plays once
        int loops = loop ? -1 : 0;
        Mix_PlayChannel(-1, it->second, loops);
    } else {
        std::cerr << "Sound not found: " << name << std::endl;
    }
}

void AudioSystem::stop(const std::string& name) {
    auto it = sounds.find(name);
    if (it != sounds.end()) {
        // find and stop the channel playing this sound
        int num_channels = Mix_AllocateChannels(-1);
        for (int channel = 0; channel < num_channels; channel++) {
            if (Mix_Playing(channel) && Mix_GetChunk(channel) == it->second) {
                Mix_HaltChannel(channel);
                break;
            }
        }
    }
}

void AudioSystem::stop_all() {
    Mix_HaltChannel(-1);
}

void AudioSystem::cleanup() {
    // Free all loaded sounds
    for (auto& pair : sounds) {
        Mix_FreeChunk(pair.second);
    }
    sounds.clear();

    Mix_CloseAudio();
    std::cout << "Audio system cleaned up" << std::endl;
}
