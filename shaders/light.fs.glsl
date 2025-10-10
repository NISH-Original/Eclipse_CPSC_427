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

// Output color
layout(location = 0) out vec4 color;

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
    
    // Combine distance and angle factors
    float light_intensity = distance_factor * angle_factor * light_brightness;
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
