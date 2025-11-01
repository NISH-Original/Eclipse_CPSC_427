#pragma once

#include "common.hpp"

#include <vector>
#include <random>

// Perlin noise generator
class PerlinNoiseGenerator {
    private:
        std::mt19937 m_rng;
        std::vector<unsigned short> permutation;
        unsigned int tot_oct;

        float raw_noise(float x, float y);
        vec2 getGradient(unsigned short perm_val);

    public:
        // initialize generator
        PerlinNoiseGenerator();
        void init(int seed = 0, unsigned int octaves = 4);

        // Get noise function value at a given position
        float noise(float x, float y);
};
