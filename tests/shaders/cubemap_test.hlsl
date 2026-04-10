#include "constants.hlsli"
struct CustomPush {
  uint cubemap;
  uint sampler;
  float paddings[kPushConstantFloatSize - 2];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "screen_space.hlsli"

VsOutput VsMain(uint vertexId : SV_VertexID) {
  return fullscreenSkybox(vertexId, uCamera.invProjection, uCamera.invView);
}

float4 FsMain(VsOutput input) : SV_Target {
  return float4(sampleCubeLod(push.cubemap, 0, input.viewDir, 1.0).rgb, 1);
}
