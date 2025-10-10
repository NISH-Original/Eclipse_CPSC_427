#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform int total_frame = 6; // Hardcoded values for now
uniform int curr_frame = 3;

void main()
{
	float frameWidth = 1.0f / float(total_frame);
	texcoord.x = (float(curr_frame) + in_texcoord.x) * frameWidth;
	texcoord.y = in_texcoord.y;

	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}