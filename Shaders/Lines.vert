
#version 450

layout(location=0) in vec3 pos;

layout(set=0, binding=4) uniform UniformBufferObject {
  mat4 projection;
  mat4 inverseProjection;
  mat4 view;
  mat4 inverseView;
  vec3 lightDir;
  float time;
  float exposure;
} globals;

void main() {
  gl_Position = globals.projection * globals.view * vec4(pos, 1.0);;
}