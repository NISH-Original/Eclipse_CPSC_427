#version 330

uniform sampler2D screen_texture;
uniform float time;
uniform float darken_screen_factor;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE WATER DISTORTION HERE (you may want to try sin/cos)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	float wave_x = sin(uv.y * 6.0 + time * 0.3) * 0.008;
	float wave_y = sin(uv.x * 5.0 + time * 0.2) * 0.006;
	
	// boundary handling
	vec2 distorted_uv = uv + vec2(wave_x, wave_y);
	
	// prevent seam artifacts
	float edge_fade = min(uv.x, 1.0 - uv.x) * min(uv.y, 1.0 - uv.y) * 8.0;
	edge_fade = clamp(edge_fade, 0.0, 1.0);
	distorted_uv = mix(uv, distorted_uv, edge_fade);
	
	distorted_uv = clamp(distorted_uv, 0.0, 1.0);
	
	return distorted_uv;
}

vec4 color_shift(vec4 in_color) 
{
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO A1: HANDLE THE COLOR SHIFTING HERE (you may want to make it blue-ish)
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	// Shift slightly towards blue
	vec4 shifted_color = in_color;
	shifted_color.b += 0.2; // add blue
	
	// Clamp values to valid range
	shifted_color.rgb = clamp(shifted_color.rgb, 0.0, 1.0);
	
	return shifted_color;
}

vec4 fade_color(vec4 in_color) 
{
	if (darken_screen_factor > 0)
		in_color -= darken_screen_factor * vec4(0.8, 0.8, 0.8, 0);
	return in_color;
}

void main()
{
	vec2 coord = distort(texcoord);

    vec4 in_color = texture(screen_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}