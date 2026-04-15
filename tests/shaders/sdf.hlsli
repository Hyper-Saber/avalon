#ifndef AVALON_SDF_HLSLI
#define AVALON_SDF_HLSLI

#include "common.hlsli"
#include "noise.hlsli"

float sdfShpere(float3 p, float r) {
  float d2 = dot(p, p);
  float r2 = r * r;

  return (d2 - r2) / (length(p) + r);
}

float sdfBox(float3 p, float3 s) {
  float3 q = abs(p) - s;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

float sdfBox2D(float2 p, float2 b) {
  float2 d = abs(p) - b;
  return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

float sdf(uint type, float3 extent, float3 p) {
  switch(type) {
  case kCube:
    return sdfBox(p, extent);
  case kPlane:
    return max(sdfBox2D(p.xz, extent.xz), abs(p.y) - 0.001);
  case kQuad:
    return max(sdfBox2D(p.xy, extent.xy), abs(p.z) - 0.001);
  case kSphere:
    return sdfShpere(p, extent.x);
  default:
    return 1e5;
  }
}

float3 calcNumericNormal(uint type, float3 extent, float3 p) {
  float2 e = float2(0.005, 0.0);
  return normalize(
      float3(sdf(type, extent, p + e.xyy) - sdf(type, extent, p - e.xyy),
             sdf(type, extent, p + e.yxy) - sdf(type, extent, p - e.yxy),
             sdf(type, extent, p + e.yyx) - sdf(type, extent, p - e.yyx)));
}

float getShapeBias(uint type) {
  switch(type) {
  case kPlane:
  case kQuad:
    return 0.04;
  case kCube:
    return 0.02;
  case kSphere:
    return 0.01;
  default:
    return 0.01;
  }
}

float getSceneSDF(float3 worldPos, uint instanceBufferOffset,
                  uint instanceCount) {

  float minDistance = 1e6;
  for(uint j = 0; j < instanceCount; j++) {
    InstanceData data = loadInstanceData(instanceBufferOffset, j);
    float3 localPos =
        worldToLocal(data.invModelOffset, float4(worldPos, 1.0)).xyz;
    float distance = sdf(data.sdfType, data.sdfExtent, localPos);

    minDistance = min(minDistance, distance);
  }

  return minDistance;
}

float calculateSDFAO(float3 p, float3 n, uint instanceBufferOffset,
                     uint instanceCount) {
  float occ = 0.0;
  float weight = 1.0;

  for(uint i = 1; i <= 5; i++) {
    float t = 0.01 + 0.12 * float(i) / 5.0;

    float d = getSceneSDF(p + n * t, instanceBufferOffset, instanceCount);

    occ += weight * (t - d);
    weight *= 0.5;
  }

  return saturate(1.0 - 5.0 * occ);
}

float shadowMarch(float3 rayOrigin, float3 rayDirection, float maxDistance,
                  float maxStep, float k, uint instanceBufferOffset,
                  uint instanceCount) {
  float t = 0.02;
  float res = 1.0;
  float ph = 1e10;
  maxStep = 128;

  for(uint i = 0; i < maxStep; i++) {
    float3 currPos = rayOrigin + t * rayDirection;
    float h = getSceneSDF(currPos, instanceBufferOffset, instanceCount);
    if(h < 0.001)
      return 0.0;

    float y = h * h / (2.0 * ph);
    float d = sqrt(h * h - y * y);
    res = min(res, k * d / max(0.0, t - y));
    ph = h;
    t += h;
  }

  return saturate(res);
}

#endif
