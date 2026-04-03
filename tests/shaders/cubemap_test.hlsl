struct CustomPush {
  uint cubeTexture;
  uint padding[6];
};
#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "screen_space.hlsli"

VsOutput VsMain(uint vertexId : SV_VertexID) {
  return FullscreenSkybox(vertexId, uCamera.invProjection, uCamera.invView);
}

float4 FsMain(VsOutput input) : SV_Target {
  float3 dir = normalize(input.viewDir);

  float4 hdrColor = sampleCube(push.custom.cubeTexture, mMaterial.sampler, dir);
  // float4 ldrColor = hdrColor / (hdrColor + 1.0);
  // return ldrColor;
  return hdrColor;
}
