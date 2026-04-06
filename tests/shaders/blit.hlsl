struct BlitCustom {
  uint sceneColor;
  uint padding[6];
};

#define CUSTOM_PUSH_TYPE BlitCustom
#include "common.hlsli"
#include "screen_space.hlsli"

VsOutput VsMain(uint vertexID : SV_VertexID) {
  return FullscreenBase(vertexID);
}

float4 FsMain(VsOutput input) : SV_Target {
  float4 color =
      sampleTexture2d(push.custom.sceneColor, mMaterial.sampler, input.uv);
  return color;
}
