
#version 450

// Per-instance attributes
layout(location=0) in vec3 instancePos;
layout(location=1) in float radius;
layout(location=2) in vec3 color;
layout(location=3) in float roughness;
layout(location=4) in float metallic;

// Per-vertex attributes
layout(location=5) in vec3 vertPos;

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
layout(location=2) out vec3 colorOut;
layout(location=3) out float roughnessOut;
layout(location=4) out float metallicOut;

void main() {
  vec3 cameraPos = globals.inverseView[3].xyz;
  worldPos = instancePos + radius * vertPos; 

  direction = worldPos - cameraPos;
  colorOut = color;
  roughnessOut = roughness;
  metallicOut = metallic;
  
  gl_Position = globals.projection * globals.view * vec4(worldPos, 1.0);;
}