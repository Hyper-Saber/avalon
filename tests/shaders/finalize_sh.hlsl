#include "constants.hlsli"
struct CustomPush {
  uint shOffset;
  uint weightOffset;
  float paddings[kPushConstantFloatSize - 2];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "pbr.hlsli"

[numthreads(16, 1, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID) {
  if(dispatchId.x > 8)
    return;
  float weight = uDynamicSSBO.Load<float>(push.weightOffset);
  if(weight < kEpsilon)
    return;

  uint address = push.shOffset + dispatchId.x * 16;
  float3 coefficient = uDynamicSSBO.Load<float3>(address);
  uDynamicSSBO.Store3(address, asuint(coefficient / weight));
}
