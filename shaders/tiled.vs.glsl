#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;
//uniform mat4 tex_states;

uniform int total_states;
uniform int s_bit;

void main()
{
	// TODO: fix edge artifacts
	float stateWidth = 1.0f / float(total_states);
	float x_offset = 2.0f/512.0f;
	float y_offset = 16.0f/64.0f;
	float u = in_texcoord.x;
	float v = in_texcoord.y;
	texcoord.x = (float(s_bit - 1) + u)*stateWidth + s_bit*x_offset;
	texcoord.y = v/2 + y_offset;

	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}