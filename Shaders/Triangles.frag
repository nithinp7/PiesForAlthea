#version 450

#define PI 3.14159265359

layout(location=0) in vec3 worldPos;
layout(location=1) in vec3 direction;

layout(location=0) out vec4 color;

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

#include <PBR/PBRMaterial.frag>

void main() {
  vec3 dFdxPos = dFdx(worldPos);
  vec3 dFdyPos = dFdy(worldPos);
  vec3 normal = normalize(cross(dFdxPos,dFdyPos));

  vec3 baseColor = vec3(10.0, 0.0, 0.0);
  float roughness = 0.1;
  float metallic = 0.0;

  float ambientOcclusion = 1.0;

  vec3 reflectedDirection = reflect(normalize(direction), normal);
  vec3 reflectedColor = sampleEnvMap(reflectedDirection, roughness);
  vec3 irradianceColor = sampleIrrMap(normal);

  vec3 material = 
      pbrMaterial(
        normalize(direction),
        globals.lightDir, 
        normal, 
        baseColor.rgb, 
        reflectedColor, 
        irradianceColor,
        metallic, 
        roughness, 
        ambientOcclusion);

  material = vec3(1.0) - exp(-material * globals.exposure);
  color = vec4(material, 1.0);
}