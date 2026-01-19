#version 330

// Light inputs
uniform vec2 light_position;
uniform vec3 light_color;
uniform float light_radius;
uniform vec2 screen_size;
uniform float flicker_intensity;
uniform float time;
uniform sampler2D scene_texture;
uniform sampler2D sdf_texture;
uniform vec2 light_direction;
uniform float cone_angle;
uniform float light_height;

in vec2 texcoord;
out vec4 fragColor;

float calculateShadow(vec2 pixelPos, vec2 lightPos)
{
    vec2 toLightDir = lightPos - pixelPos;
    float distToLight = length(toLightDir);
    toLightDir = normalize(toLightDir);

    const int RINGS = 2;
    const int SAMPLES_PER_RING = 8;
    int totalSamples = 0;
    float shadow = 0.0;

    for (int ring = 0; ring < RINGS; ring++)
    {
        float ringRadius = (float(ring) + 1.0) * 4.0;
        float angleOffset = float(ring) * 0.39;

        for (int i = 0; i < SAMPLES_PER_RING; i++)
        {
            float angle = (float(i) / float(SAMPLES_PER_RING) * 6.28318) + angleOffset;
            vec2 offset = vec2(cos(angle), sin(angle)) * ringRadius;
            vec2 samplePos = pixelPos + offset;

            vec2 dir = normalize(lightPos - samplePos);
            float dist = distance(lightPos, samplePos);

            bool hit = false;
            float hitDist = dist;
            float traveled = 2.0;

            for (int step = 0; step < 32; step++)
            {
                if (traveled >= dist) break;

                vec2 testPos = samplePos + dir * traveled;
                vec2 uv = testPos / screen_size;
                uv.y = 1.0 - uv.y;

                if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
                    break;
                }

                float sdfDist = texture(sdf_texture, uv).r * max(screen_size.x, screen_size.y);

                if (sdfDist < 1.0) {
                    hit = true;
                    hitDist = traveled;
                    break;
                }

                traveled += max(sdfDist, 2.0);
            }

            if (!hit) {
                shadow += 1.0;
            } else {
                float distLightToOccluder = dist - hitDist;
                float shadowLength = distLightToOccluder * light_height;

                if (hitDist >= shadowLength) {
                    shadow += 1.0;
                }
            }

            totalSamples++;
        }
    }

    float visibility = shadow / float(totalSamples);
    return mix(0.5, 1.0, visibility);
}

void main()
{
    vec2 pixelPos = texcoord * screen_size;
    pixelPos.y = screen_size.y - pixelPos.y;

    vec2 toPixel = pixelPos - light_position;
    float dist = length(toPixel);

    if (dist > light_radius) {
        discard;
    }

    float normalized = clamp(dist / light_radius, 0.0, 1.0);

    // Intensity falloff is (1.0 - normalized)^2 for smooth falloff
    float distFactor = pow(1.0 - normalized, 1.2);

    float coneFactor = 1.0;
    if (cone_angle < 3.0) {
        // is a cone light

        // Calculate angle between light direction and pixel
        vec2 pixelDir = normalize(toPixel);
        float dotProduct = dot(pixelDir, light_direction);
        float angle = acos(dotProduct);

        float outerCone = cone_angle;
        float innerCone = cone_angle * 0.8;

        if (angle > outerCone) {
            // discard if outside cone
            discard;
        }

        // Smooth edge
        coneFactor = 1.0 - smoothstep(innerCone, outerCone, angle);
    }

    float shadow = calculateShadow(pixelPos, light_position);

    vec4 sceneColor = texture(scene_texture, texcoord);
    bool isEmpty = length(sceneColor.rgb) < 0.01;

    // If there's flickering, randomly calculate and change flickering over time
    float flickerAmount = 1.0;
    if (flicker_intensity > 0.0) {
        float flicker = 0.9 + 0.1 * sin(time * 7.234 + light_position.x * 0.011 + light_position.y * 0.013)
                             + 0.05 * cos(time * 3.112 + light_position.x * 0.21);
        flickerAmount = flicker_intensity * clamp(flicker, 0.7, 1.2);
    }

    float Factor = distFactor * coneFactor * flickerAmount * shadow * 0.6;

    vec3 result;
    if (isEmpty) {
        result = light_color * Factor;
    } else {
        result = sceneColor.rgb * light_color * Factor;
    }

    fragColor = vec4(result, 1.0);
}
