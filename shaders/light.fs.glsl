#version 330

// From vertex shader
in vec3 vertex_color;
in vec2 world_position;

// Application data
uniform vec3 fcolor;
uniform vec2 player_position;
uniform vec2 player_direction;

// Light properties (from Light component)
uniform float light_cone_angle;
uniform float light_brightness;
uniform float light_brightness_falloff;
uniform float light_range;
uniform vec3 light_color;
uniform float light_inner_cone_angle;

// Global world settings
uniform float global_ambient_brightness;

// Occlusion data
uniform sampler2D occlusion_texture;
uniform vec2 screen_size;

// Output color
layout(location = 0) out vec4 color;

vec2 worldToUV(vec2 world_pos) {
    vec2 uv = world_pos / screen_size;
    uv.y = 1.0 - uv.y;
    return uv;
}

// Simple ray march from light to fragment
float checkOcclusion(vec2 light_pos, vec2 frag_pos) {

    // Calculate distance first (for early exit)
    vec2 direction = frag_pos - light_pos;
    float dist = length(direction);
    
    // Early exit for very close fragments
    if (dist < 1.0) return 1.0;
    
    // Get pixel distance between light and fragment
    vec2 start_uv = worldToUV(light_pos);
    vec2 end_uv = worldToUV(frag_pos);
    vec2 delta_uv = end_uv - start_uv;
    vec2 delta_pixels = delta_uv * screen_size;

    // Steps = max of x diff and y diff
    int steps = int(max(abs(delta_pixels.x), abs(delta_pixels.y)));

    // March from light toward fragment
    for (int i = 1; i < steps - 1; i++) {
        float t = float(i) / float(steps);
        vec2 uv = start_uv + delta_uv * t;

        if (uv.x < 0.0 || uv.x > 1.0 
        || uv.y < 0.0 || uv.y > 1.0) continue;

        // Check if we hit an occluder
        if (texture(occlusion_texture, uv).r > 0.5) {
            return 0.0;  // In shadow
        }
    }
    
    return 1.0;  // Not occluded
}

void main()
{
    vec2 frag_pos = world_position;
    float distance_to_player = length(frag_pos - player_position);
    vec2 to_fragment = normalize(frag_pos - player_position);
    float angle_to_fragment = acos(dot(normalize(player_direction), to_fragment));

    // Calculate distance falloff with smooth transition beyond range
    float normalized_distance = distance_to_player / light_range;
    float distance_factor = 1.0 - smoothstep(0.0, 1.0, normalized_distance);
    distance_factor = pow(distance_factor, light_brightness_falloff);
    
    // Calculate angle falloff with gradual transition
    float angle_factor = 1.0;
    if (light_inner_cone_angle > 0.0) {
        angle_factor = 1.0 - smoothstep(light_inner_cone_angle, light_cone_angle, angle_to_fragment);
    } else {
        angle_factor = 1.0 - smoothstep(0.0, light_cone_angle, angle_to_fragment);
    }
    
    // Check occlusion
    float occlusion_factor = checkOcclusion(player_position, frag_pos);
    
    // Combine distance, angle, and occlusion factors
    float light_intensity = distance_factor * angle_factor * light_brightness * occlusion_factor;
    light_intensity = clamp(light_intensity, 0.0, 1.0);

    vec3 original_color = vertex_color * fcolor;
    vec3 lit_color = original_color;
    
    float ambient_factor = 1.0 - light_intensity;
    float light_factor = light_intensity;
    
    lit_color = original_color * (global_ambient_brightness * ambient_factor + light_factor) + 
                (light_color * light_intensity * 0.5);
    
    lit_color = clamp(lit_color, 0.0, 1.0);

    color = vec4(lit_color, 1.0);
}
