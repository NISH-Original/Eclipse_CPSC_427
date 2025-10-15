#version 330

// Input attributes
layout(location = 0) in vec2 in_position;
layout(location = 1) in vec4 in_color;
layout(location = 2) in vec2 in_texcoord;

// Passed to fragment shader
out vec4 frag_color;
out vec2 frag_texcoord;

// Application data
uniform mat4 projection;
uniform vec2 translation;

void main()
{
	frag_color = in_color;
	frag_texcoord = in_texcoord;
	vec2 pos = in_position + translation;
	gl_Position = projection * vec4(pos, 0.0, 1.0);
}

