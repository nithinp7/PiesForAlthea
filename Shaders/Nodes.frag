#version 450

#define PI 3.14159265359

layout(location=0) in vec3 worldPos;
layout(location=1) in vec3 direction;
layout(location=2) in vec3 baseColor;
layout(location=3) in float roughness;
layout(location=4) in float metallic;
layout(location=5) in vec3 normal;

layout(location=0) out vec4 GBuffer_Position;
layout(location=1) out vec4 GBuffer_Normal;
layout(location=2) out vec4 GBuffer_Albedo;
layout(location=3) out vec4 GBuffer_MetallicRoughnessOcclusion;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

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
  GBuffer_Position = vec4(worldPos, 1.0);
  GBuffer_Albedo = vec4(baseColor, 1.0);
  GBuffer_MetallicRoughnessOcclusion = vec4(metallic, roughness, 1.0, 1.0);

  vec3 N = normal;
  if (dot(direction, N) > 0.0) {
    N *= -1.0;
  }
  
  GBuffer_Normal = vec4(N, 1.0);
}