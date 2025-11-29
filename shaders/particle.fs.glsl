#version 330

in vec4 Color;
out vec4 FragColor;

void main()
{
    if (Color.a < 0.01) discard;
    FragColor = Color;
}
