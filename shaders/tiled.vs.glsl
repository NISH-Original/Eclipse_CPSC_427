#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform mat4 tex_states;
uniform int total_states;

void main()
{
	float stateWidth = 1.0f / float(total_states);

	int i = int(in_texcoord.x * 4);
	int j = int(in_texcoord.y * 4);
	float u = mod(in_texcoord.x * 4, 1.0);
	float v = mod(in_texcoord.y * 4, 1.0);
	if (i == 4) {
		i = 3;
		u = 1.0;
	}
	if (j == 4) {
		j = 3;
		v = 1.0;
	}
	float offset = tex_states[i][j];

	texcoord.x = (float(offset) + u) * stateWidth;
	texcoord.y = v;

	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}