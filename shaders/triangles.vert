#version 400

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std140, binding = 0) uniform buffer {
  mat4 mvp;
  vec4 colors[4];
} uniform_buffer;

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 norm;
layout (location = 2) in uint color_index;

layout (location = 0) flat out vec4 outColor;

void main() {
  outColor = uniform_buffer.colors[color_index & 0x3];
  gl_Position = uniform_buffer.mvp * pos;
}