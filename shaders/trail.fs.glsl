#version 330

in vec2 texcoord;
uniform sampler2D sprite_tex;

uniform float u_alpha;
uniform int u_colorMode;

out vec4 color;

void main() {
    vec4 base = texture(sprite_tex, texcoord);

    if (base.a < 0.01)
        discard;

    vec2 centered = texcoord - vec2(0.5);
    float dist = length(centered);

    float fog = exp(-4.0 * dist * dist); 
    vec3 blueFog  = vec3(0.25, 0.6, 1.0);
    vec3 redFog   = vec3(1.0, 0.3, 0.3);
    vec3 fogColor = (u_colorMode == 1) ? redFog : blueFog;
    float finalAlpha = fog * u_alpha * 1.5;

    color = vec4(fogColor, finalAlpha);
}
