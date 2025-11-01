#version 330

// Converted from: https://github.com/Hybrid46/RadianceCascade2DGlobalIllumination
// Jump Flood Algorithm
// Builds Voronoi diagram

in vec2 texcoord;
out vec4 fragColor;

uniform sampler2D previous_texture;  // Contains UVs from previous pass
uniform float step_size;             // Current step size (halves each iteration)
uniform vec2 aspect;                 // Screen aspect ratio correction

void main()
{
    float minDist = 1e10;
    vec2 bestUV = vec2(0.0);

    // Check 3x3 area at step_size distance
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            // Sample neighbor at step_size pixels away
            vec2 offset = vec2(x, y) * aspect.yx * step_size;
            vec2 peekUV = texcoord + offset;
            vec2 peek = texture(previous_texture, peekUV).xy;

            // Skip empty pixels
            if (peek.x != 0.0 && peek.y != 0.0)
            {
                // Calculate distance to this seed
                vec2 dir = peek - texcoord;
                float d = dot(dir, dir); 

                if (d < minDist)
                {
                    minDist = d;
                    bestUV = peek;
                }
            }
        }
    }

    // Write the nearest seed UV
    fragColor = vec4(bestUV, 0.0, 1.0);
}
