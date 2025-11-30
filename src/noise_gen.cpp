#include "noise_gen.hpp"

const unsigned short PERMUTATION_SCALE = 10;
const unsigned short PERMUTATION_LENGTH = 1 << PERMUTATION_SCALE;

// NOTE: Perlin noise implementation based on ideas and notes from various authors
// - 2002 paper by Perlin about improvements to the algorithm: https://doi.org/10.1145/566654.566636
// - Archived talk by Perlin about the original algorithm: https://web.archive.org/web/20071008162042/http://www.noisemachine.com/talk1/index.html
//   - Sourced from https://en.wikipedia.org/wiki/Perlin_noise
// - Algorithm explanation by Raouf (including reference implementation): https://rtouti.github.io/graphics/perlin-noise-algorithm
// - Algorithm explanation by Adrian (including reference implementation): https://adrianb.io/2014/08/09/perlinnoise.html

// Noise generator functions
PerlinNoiseGenerator::PerlinNoiseGenerator() {}

void PerlinNoiseGenerator::init(unsigned int seed, unsigned int octaves) {
	this->tot_oct = octaves;
	std::mt19937 m_rng(seed);
	permutation.clear();
	for (unsigned short i = 0; i < PERMUTATION_LENGTH; i++) {
        permutation.push_back(i);
    }

    // randomize permutation based on seed
    for (unsigned short i = 0; i < PERMUTATION_LENGTH - 1; i++) {
		// NOTE: using raw data to produce reproducible results
		unsigned int raw_num = m_rng();
        unsigned short j = (unsigned short) (raw_num % (PERMUTATION_LENGTH - i)) + i;
        if (j == PERMUTATION_LENGTH)
            j -= 1;

		std::swap(permutation[i], permutation[j]);
    }
}

// Improved method (from 2002 Perlin paper): get one of 4 consident gradients
// https://doi.org/10.1145/566654.566636
vec2 PerlinNoiseGenerator::getGradient(unsigned short perm_val) {
	switch (perm_val % 4) {
		case 0: return vec2(1, 1);
		case 1: return vec2(1, -1);
		case 2: return vec2(-1, 1);
		default: return vec2(-1, -1);
	}
}

float PerlinNoiseGenerator::raw_noise(float x, float y) {
	// get nearest integers x, x+1, y, and y+1 (mod PERMUTATION_LENGTH)
	float floor_x = std::fmod(floor(x), PERMUTATION_LENGTH);
	float floor_y = std::fmod(floor(y), PERMUTATION_LENGTH);
	if (floor_x < 0)
		floor_x += PERMUTATION_LENGTH;
	if (floor_y < 0)
		floor_y += PERMUTATION_LENGTH;

	unsigned short x_0 = (unsigned short) floor_x;
	unsigned short x_1 = (unsigned short) (x_0 + 1) % PERMUTATION_LENGTH;
	unsigned short y_0 = (unsigned short) floor_y % PERMUTATION_LENGTH;
	unsigned short y_1 = (unsigned short) (y_0 + 1) % PERMUTATION_LENGTH;

	// hash coordinates to gradient lookup values, then get gradients
	vec2 ul = getGradient(permutation[(permutation[x_0] + y_0) % PERMUTATION_LENGTH]);
	vec2 ur = getGradient(permutation[(permutation[x_1] + y_0) % PERMUTATION_LENGTH]);
	vec2 dl = getGradient(permutation[(permutation[x_0] + y_1) % PERMUTATION_LENGTH]);
	vec2 dr = getGradient(permutation[(permutation[x_1] + y_1) % PERMUTATION_LENGTH]);

	// compute dot products of gradients w/ distances to input point
	float xr = std::fmod(x, 1);
	float yr = std::fmod(y, 1);
	if (xr < 0)
		xr += 1;
	if (yr < 0)
		yr += 1;
	float ul_dp = dot(ul, vec2(xr, yr));
	float ur_dp = dot(ur, vec2(1-xr, yr));
	float dl_dp = dot(dl, vec2(xr, 1-yr));
	float dr_dp = dot(dr, vec2(1-xr, 1-yr));

	// compute interpolation function values
	// 6t^5 - 15t^4 + 10t^3  =  t^3 * (6t^2 - 15t + 10)
	float x_interp = xr * xr * xr * ((6 * xr - 15) * xr + 10);
	float y_interp = yr * yr * yr * ((6 * yr - 15) * yr + 10);

	// interpolate between dot products
	float upper = (1-x_interp)*ul_dp + x_interp*ur_dp;
	float lower = (1-x_interp)*dl_dp + x_interp*dr_dp;
	return (1-y_interp)*upper + y_interp*lower;
}

// Sum multiple octaves of perlin noise, and normalize result
float PerlinNoiseGenerator::noise(float x, float y) {
	float noise_val = 0;
	float amp = 0;
	float curr_scale = 1;
	for (unsigned int i = 1; i <= tot_oct; i++) {
		noise_val += raw_noise((float) curr_scale * x, (float) curr_scale * y) / (float) curr_scale;
		amp += M_SQRT_2 / curr_scale;
		curr_scale *= 2;
	}
    return noise_val / amp;
}
