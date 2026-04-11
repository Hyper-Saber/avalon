#include "constants.hlsli"
struct BlitCustom {
  uint sceneColor;
  uint sampler;
  uint padding[kPushConstantFloatSize - 2];
};

#define CUSTOM_PUSH_TYPE BlitCustom
#include "common.hlsli"
#include "screen_space.hlsli"

VSOutput VsMain(uint vertexID : SV_VertexID) {
  return fullscreenBase(vertexID);
}

float4 FsMain(VSOutput input) : SV_Target {
  float4 color = sampleTexture2d(push.sceneColor, push.sampler, input.uv);
  return color;
}
