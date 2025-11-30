#version 330

// Converted from: https://github.com/Hybrid46/RadianceCascade2DGlobalIllumination

in vec2 texcoord;
out vec4 fragColor;

uniform sampler2D scene_texture;

void main()
{
    vec4 scene = texture(scene_texture, texcoord);
    bool hasScene = scene.a > 0.5;

    if (hasScene)
    {
        // Mark all scene objects as shadow casters by outputting UV coordinates
        fragColor = vec4(texcoord, 0.0, 1.0);
    }
    else
    {
        fragColor = vec4(0.0);
    }
}
