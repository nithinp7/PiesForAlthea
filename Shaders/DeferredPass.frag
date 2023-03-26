#version 450

#define PI 3.14159265359

layout(location=0) in vec3 direction;
layout(location=1) in vec2 uv;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D environmentMap; 
layout(set=0, binding=1) uniform sampler2D prefilteredMap; 
layout(set=0, binding=2) uniform sampler2D irradianceMap;
layout(set=0, binding=3) uniform sampler2D brdfLut;

#define GLOBAL_UNIFORMS_SET 0
#define GLOBAL_UNIFORMS_BINDING 4
#include <GlobalUniforms.glsl>

// GBuffer textures
layout(set=1, binding=0) uniform sampler2D gBufferPosition;
layout(set=1, binding=1) uniform sampler2D gBufferNormal;
layout(set=1, binding=2) uniform sampler2D gBufferAlbedo;
layout(set=1, binding=3) uniform sampler2D gBufferMetallicRoughnessOcclusion;

// Prefiltered reflection buffer
layout(set=1, binding=4) uniform sampler2D reflectionBuffer;

#include <PBR/PBRMaterial.glsl>

vec4 sampleReflection(float roughness) {
  return textureLod(reflectionBuffer, uv, 4.0 * roughness).rgba;
} 

vec3 sampleEnvMap(vec3 dir) {
  float yaw = atan(dir.z, dir.x);
  float pitch = -atan(dir.y, length(dir.xz));
  vec2 envMapUV = vec2(0.5 * yaw, pitch) / PI + 0.5;

  return textureLod(environmentMap, envMapUV, 0.0).rgb;
} 

// Random number generator and sample warping
// from ShaderToy https://www.shadertoy.com/view/4tXyWN
// TODO: Find better alternative??
uvec2 seed;
float rng() {
    seed += uvec2(1);
    uvec2 q = 1103515245U * ( (seed >> 1U) ^ (seed.yx) );
    uint  n = 1103515245U * ( (q.x) ^ (q.y >> 3U) );
    return float(n) * (1.0 / float(0xffffffffU));
}

// Useful functions for transforming directions
// TODO: check if the exact tangent, bitangent coords matter
void coordinateSystem(in vec3 v1, out vec3 v2, out vec3 v3) {
    if (abs(v1.x) > abs(v1.y))
            v2 = vec3(-v1.z, 0, v1.x) / sqrt(v1.x * v1.x + v1.z * v1.z);
        else
            v2 = vec3(0, v1.z, -v1.y) / sqrt(v1.y * v1.y + v1.z * v1.z);
        v3 = cross(v1, v2);
}

mat3 LocalToWorld(vec3 nor) {
  vec3 tan, bit;
  coordinateSystem(nor, tan, bit);
  return mat3(tan, bit, nor);
}

#define ENABLE_SSAO
#ifdef ENABLE_SSAO 
#define SSAO_RAY_COUNT 24
#define SSAO_RAYMARCH_STEPS 12
float computeSSAO(vec2 uvStart, vec3 worldPos, vec3 normal) {
  float ao = 0;

  mat3 localToWorld = LocalToWorld(normal);

  for (int raySample = 0; raySample < SSAO_RAY_COUNT; ++raySample) {
    vec3 xi = vec3(rng(), rng(), rng());
    vec3 rayDir = localToWorld * normalize(vec3(2.0 * xi.xy - vec2(1.0), xi.z));
    
    // Arbitrary
    vec3 endPos = worldPos + rayDir * 0.5;
    vec4 projectedEnd = globals.projection * globals.view * vec4(endPos, 1.0);
    vec2 uvEnd = 0.5 * projectedEnd.xy / projectedEnd.w + vec2(0.5);

    // Should probably create a local 2D tangent space along raydir
    vec3 perpRef = cross(rayDir, normal);
    perpRef = normalize(cross(perpRef, rayDir));
    
    vec3 prevPos = worldPos;
    float prevProjection = 0.0;

    for (int i = 0; i < SSAO_RAYMARCH_STEPS; ++i) {
      vec2 currentUV = mix(uvStart, uvEnd, i / float(SSAO_RAYMARCH_STEPS));
      if (currentUV.x < 0.0 || currentUV.x > 1.0 || currentUV.y < 0.0 || currentUV.y > 1.0) {
        break;
      }

      // TODO: Check for invalid position
      
      vec3 currentPos = textureLod(gBufferPosition, currentUV, 0.0).xyz;
      vec3 dir = currentPos - worldPos;
      float currentProjection = dot(dir, perpRef);

      float dist = length(dir);
      dir = dir / dist;

      // TODO: interpolate between last two samples
      // Step between this and the previous sample
      float worldStep = length(currentPos - prevPos);
      
      if (currentProjection * prevProjection < 0.0 && worldStep <= 2.0 && i > 0) {
        vec3 currentNormal = normalize(textureLod(gBufferNormal, currentUV, 0.0).xyz);
        if (dot(currentNormal, rayDir) < 0) {
          ao += 1.0;
          break;
        }
      }

      prevPos = currentPos;
      prevProjection = currentProjection;
    }
  }

  return 1.0 - ao / SSAO_RAY_COUNT;
}
#endif

void main() {
  seed = uvec2(gl_FragCoord.xy);

  vec4 position = texture(gBufferPosition, uv).rgba;
  if (position.a == 0.0) {
    // Nothing in the GBuffer, draw the environment map
    vec3 envMapSample = sampleEnvMap(direction);
#ifndef SKIP_TONEMAP
    envMapSample = vec3(1.0) - exp(-envMapSample * globals.exposure);
#endif
    outColor = vec4(envMapSample, 1.0);
    return;
  }

  vec3 normal = normalize(texture(gBufferNormal, uv).xyz);
  vec3 baseColor = texture(gBufferAlbedo, uv).rgb;
  vec3 metallicRoughnessOcclusion = //vec3(0.0, 0.2, 0.0);
      texture(gBufferMetallicRoughnessOcclusion, uv).rgb;

  vec3 reflectedDirection = reflect(normalize(direction), normal);
  vec4 reflectedColor = sampleReflection(metallicRoughnessOcclusion.y);
  vec4 envReflectedColor = vec4(sampleEnvMap(reflectedDirection, metallicRoughnessOcclusion.y), 1.0);
  if (reflectedColor.a < 0.01) {
    reflectedColor.rgb = envReflectedColor.rgb;
  } else {
    reflectedColor.rgb = 
        mix(envReflectedColor.rgb, reflectedColor.rgb / reflectedColor.a, reflectedColor.a);
  }

  vec3 irradianceColor = sampleIrrMap(normal);

#ifdef ENABLE_SSAO
  metallicRoughnessOcclusion.z = computeSSAO(uv, position.xyz, normal);
#endif

  vec3 material = 
      pbrMaterial(
        normalize(direction),
        globals.lightDir, 
        normal, 
        baseColor.rgb, 
        reflectedColor.rgb, 
        irradianceColor,
        metallicRoughnessOcclusion.x, 
        metallicRoughnessOcclusion.y, 
        metallicRoughnessOcclusion.z);

#ifndef SKIP_TONEMAP
  material = vec3(1.0) - exp(-material * globals.exposure);
#endif

  // outColor = vec4(0.5 * direction + vec3(0.5), 1.0);

  outColor = vec4(material, 1.0);
  // outColor = vec4(vec3(metallicRoughnessOcclusion.z), 1.0);
}