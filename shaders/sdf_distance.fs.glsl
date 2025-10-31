#version 330


// Converted from: https://github.com/Hybrid46/RadianceCascade2DGlobalIllumination
// Distance Field Generation
// Converts Voronoi UVs into a Signed Distance Field

// Effectively a map of the distance to the nearest occluder
// So raymarching can be done more efficiently

in vec2 texcoord;
out vec4 fragColor;

uniform sampler2D voronoi_texture;  // Contains nearest occluder UVs

void main()
{
    // Read the nearest occluder UV from Voronoi diagram
    vec2 nearestOccluderUV = texture(voronoi_texture, texcoord).rg;

    // Calculate Euclidean distance in screen space
    vec2 currentPos = texcoord;
    vec2 occluderPos = nearestOccluderUV;
    float dist = distance(currentPos, occluderPos);

    // Store distance in red channel
    fragColor = vec4(dist, 0.0, 0.0, 1.0);
}
