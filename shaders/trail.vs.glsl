#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;

uniform mat3 transform;
uniform mat3 projection;

out vec2 texcoord;

void main() {
  texcoord = in_texcoord;
  gl_Position = vec4((projection * transform * vec3(in_position.xy, 1.0)).xy, 0.0, 1.0);
}
