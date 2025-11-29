#version 330

layout(location = 0) in vec3 in_position;

layout(location = 2) in vec3 instance_pos;
layout(location = 3) in float instance_size;
layout(location = 4) in vec4 instance_color;

out vec4 Color;

uniform mat3 projection;

void main()
{
    vec3 world_pos = vec3(
        instance_pos.x + in_position.x * instance_size * 0.3,
        instance_pos.y + in_position.y * instance_size * 1.0,
        1.0
    );

    gl_Position = vec4(projection * world_pos, 1.0);
    Color = instance_color;
}
