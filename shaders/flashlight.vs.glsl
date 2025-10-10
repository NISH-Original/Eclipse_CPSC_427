#version 330

// Input attributes
in vec3 in_position;
in vec3 in_color;

// Passed to fragment shader
out vec3 vertex_color;

// Application data
uniform mat3 transform;
uniform mat3 projection;

void main()
{
    vertex_color = in_color;
    vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
    gl_Position = vec4(pos.xy, in_position.z, 1.0);
}
