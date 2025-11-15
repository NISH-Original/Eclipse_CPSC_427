#version 330

in vec3 color;
uniform vec3 fcolor;
uniform bool is_hurt;

// Output color
layout(location = 0) out vec4 out_color;

void main()
{
	if (is_hurt) {
		out_color = vec4(1.0, 0.2, 0.2, 1.0);
	} else {
		out_color = vec4(color * fcolor, 1.0);
	}
}