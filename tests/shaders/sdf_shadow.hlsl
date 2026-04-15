#include "constants.hlsli"
#include "noise.hlsli"

struct PushConstant {
  uint depthTextureIndex;
  uint normalTextureIndex;
  uint instanceIdTextureIndex;
  uint shadowMaskIndex;

  uint opaqueInstanceBufferOffset;
  uint width;
  uint height;
  float invWidth;

  float invHeight;
  float minDistance;
  float maxDistance;
  uint maxStep;

  float k;
  uint opaqueInstanceCount;

  float paddings[kPushConstantFloatSize - 14];
};
#define CUSTOM_PUSH_TYPE PushConstant
#include "common.hlsli"
#include "sdf.hlsli"

[numthreads(8, 8, 1)] void CsMain(uint3 id : SV_DispatchThreadID) {
  if(id.x >= push.width || id.y >= push.height)
    return;

  float2 uv = (id.xy + 0.5) * float2(push.invWidth, push.invHeight);
  float depth = loadTexture2d(push.depthTextureIndex, id.xy).x;

  if(depth < kEpsilon) {
    writeRWTexture2d(push.shadowMaskIndex, id.xy, float4(1, 0, 0, 0));
    return;
  }

  uint instanceID = asuint(loadTexture2d(push.instanceIdTextureIndex, id.xy).x);
  InstanceData data =
      loadInstanceData(push.opaqueInstanceBufferOffset, instanceID);

  float2 st = uv * 2 - 1.0;

  float4 worldPos = ndcToWorld(float4(st, depth, 1.0));
  float3 worldNormal = loadTexture2d(push.normalTextureIndex, id.xy).xyz;
  float3 lightDir = -uMainLight.posDir.xyz;
  lightDir = normalize(lightDir);

  float bias = getShapeBias(data.sdfType);
  float3 origin = worldPos.xyz + bias * worldNormal;

  float aoBias = 0.005;
  float3 aoOrigin = worldPos.xyz + aoBias * worldNormal;

  float r =
      shadowMarch(origin, lightDir, push.maxDistance, push.maxStep, push.k,
                  push.opaqueInstanceBufferOffset, push.opaqueInstanceCount);

  float ao =
      calculateSDFAO(aoOrigin, worldNormal, push.opaqueInstanceBufferOffset,
                     push.opaqueInstanceCount);

  writeRWTexture2d(push.shadowMaskIndex, id.xy, float4(r, ao, 0, 0));
}
