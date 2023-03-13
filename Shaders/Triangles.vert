
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

layout(location=0) out vec3 worldPos;
layout(location=1) out vec3 direction;

void main() {
  vec3 cameraPos = globals.inverseView[3].xyz;
  direction = pos - cameraPos;

  worldPos = pos;
  gl_Position = globals.projection * globals.view * vec4(pos, 1.0);;
}