#version 330

// From vertex shader
in vec4 frag_color;
in vec2 frag_texcoord;

// Application data
uniform sampler2D tex;
uniform bool use_texture;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	if (use_texture) {
		color = frag_color * texture(tex, frag_texcoord);
	} else {
		color = frag_color;
	}
}

