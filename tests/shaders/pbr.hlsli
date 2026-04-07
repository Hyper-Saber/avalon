#ifndef AVALON_PBR_HLSLI
#define AVALON_PBR_HLSLI
#include "common.hlsli"

float distributionGGX(float3 N, float3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  float nom = a2;
  float denom = NdotH2 * (a2 - 1.0) + 1.0;
  denom = kPi * denom * denom;

  return nom / max(denom, 1e-8);
}

float geometrySchlickGGX(float NdotV, float roughness) {
  float r = roughness + 1.0;
  float k = r * r / 8.0;

  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k + kEpsilon;
  return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
  float ggx2 = geometrySchlickGGX(NdotV, roughness);
  float ggx1 = geometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

float3 fresnelSchlick(float NdotV, float3 F0) {
  return F0 + (1.0 - F0) * pow(1.0 - NdotV, 5.0);
}

float3 fresnelSchlickRoughness(float NdotV, float3 F0, float roughness) {
  return F0 + (max(1.0 - roughness, F0) - F0) * pow(1.0 - NdotV, 5.0);
}

float2 hammersley(uint i, uint N) {
  uint bits = (i << 16u) | (i >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  float rbit = float(bits) * 2.3283064365386963e-10;
  return float2(float(i) / float(N), rbit);
}

float3 importanceSampleGGX(float2 Xi, float3 N, float roughness) {
  float a = roughness * roughness;
  float phi = 2.0 * kPi * Xi.x;
  float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  float3 H;
  H.x = cos(phi) * sinTheta;
  H.y = sin(phi) * sinTheta;
  H.z = cosTheta;

  float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
  float3 tangent = normalize(cross(up, N));
  float3 bitangent = cross(N, tangent);

  return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

#endif
