#include "constants.hlsli"
struct CustomPush {
  uint cubemap;
  uint sampler;
  float paddings[kPushConstantFloatSize - 2];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "screen_space.hlsli"

VSOutput VsMain(uint vertexId : SV_VertexID) {
  return fullscreenSkybox(vertexId, uCamera.invProjection, uCamera.invView);
}

float4 FsMain(VSOutput input) : SV_Target {
  return float4(sampleCubeLod(push.cubemap, 0, input.viewDir, 1.0).rgb, 1);
}
