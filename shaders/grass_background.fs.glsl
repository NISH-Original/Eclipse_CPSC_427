#version 330

uniform sampler2D u_grass;
uniform vec2 u_camera;
uniform vec2 u_resolution;
uniform float u_tileSize;

layout(location = 0) out vec4 color;

// Hash function for deterministic pseudo-random values
float hash(vec2 p) {
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

// Get rotation angle for a tile (0, 90, 180, or 270 degrees)
float getTileRotation(vec2 tileCoord) {
	float rand = hash(floor(tileCoord));
	float rotationIndex = floor(rand * 4.0);
	return rotationIndex * 1.57079632679;
}

// Rotate UV coordinates around center (0.5, 0.5)
vec2 rotateUV(vec2 uv, float angle) {
	vec2 centered = uv - 0.5;
	float c = cos(angle);
	float s = sin(angle);
	vec2 rotated = vec2(
		centered.x * c - centered.y * s,
		centered.x * s + centered.y * c
	);
	return rotated + 0.5;
}

void main()
{
	vec2 screenPos = vec2(gl_FragCoord.x, u_resolution.y - gl_FragCoord.y);
	vec2 worldPos = u_camera + (screenPos - u_resolution * 0.5);
	vec2 tileCoord = worldPos / (u_tileSize * 2.0);
	float rotation = getTileRotation(tileCoord);
	vec2 uv = fract(tileCoord);
	uv = rotateUV(uv, rotation);
	
    color = texture(u_grass, uv);
    color.a = 0.0;
}

