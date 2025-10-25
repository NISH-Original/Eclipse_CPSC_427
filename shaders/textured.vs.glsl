#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform int total_frame;
uniform int curr_frame;
uniform int total_row;
uniform int curr_row;
uniform bool should_flip;

void main()
{
	float frameWidth = 1.0f / float(total_frame);
	float frameHeight = 1.0f / float(total_row);

	float u = in_texcoord.x;
	float v = in_texcoord.y;

	if (should_flip){
		u = 1.0f - u;
	}

	texcoord.x = (float(curr_frame) + u) * frameWidth;
	texcoord.y = (float(curr_row) + v) * frameHeight;

	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}