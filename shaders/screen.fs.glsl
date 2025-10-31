#version 330

uniform sampler2D screen_texture;

in vec2 texcoord;

layout(location = 0) out vec4 color;

void main()
{
    vec2 uv = clamp(texcoord, 0.0, 1.0);
    color = texture(screen_texture, uv);
}
