#version 330

in vec3 color;
uniform vec3 fcolor;
uniform float alpha = 1.0;

// Output color
layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(color * fcolor, alpha);
}

