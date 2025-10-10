#version 330

// From vertex shader
in vec3 vertex_color;

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
    vec2 frag_pos = gl_FragCoord.xy;
    
    float distance_to_player = length(frag_pos - player_position);
    
    vec2 to_fragment = normalize(frag_pos - player_position);
    float angle_to_fragment = acos(dot(normalize(player_direction), to_fragment));
    
    bool in_cone = angle_to_fragment <= light_cone_angle;
    bool in_range = distance_to_player <= light_range;
    
    float light_intensity = 0.0;
    if (in_cone && in_range) {
        float distance_factor = 1.0 - pow(distance_to_player / light_range, light_brightness_falloff);
        
        float angle_factor = 1.0;
        if (light_inner_cone_angle > 0.0) {
            // Soft edge: smooth transition from inner to outer cone
            angle_factor = 1.0 - smoothstep(light_inner_cone_angle, light_cone_angle, angle_to_fragment);
        } else {
            // Hard edge: sharp cutoff at cone boundary
            angle_factor = 1.0 - (angle_to_fragment / light_cone_angle);
        }
        
        light_intensity = distance_factor * angle_factor * light_brightness;
        light_intensity = clamp(light_intensity, 0.0, 1.0);
    }
    
    vec3 lit_color = vertex_color * fcolor;
    
    lit_color = lit_color * light_color * light_intensity;
    
    vec3 ambient = vec3(global_ambient_brightness);
    lit_color = max(lit_color, ambient);
    
    color = vec4(lit_color, 1.0);
}