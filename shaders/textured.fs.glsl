#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform bool is_hurt;
uniform float alpha_mod = 1.0;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	vec4 tex = texture(sampler0, texcoord);

	if (is_hurt) {
		color = vec4(1.0, 0.2, 0.2, tex.a * alpha_mod);
	} else {
		color = vec4(tex.rgb, tex.a * alpha_mod);
	}
}