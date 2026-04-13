#include "constants.hlsli"
sturct PushConstant {
  uint depthTextureIndex;
  uint instanceIdTextureIndex;
  uint sdfDataOffset;
  uint shadowMaskIndex;
  uint instanceCount;
  uint width;
  uint height;
  float invWidth;
  float invHeight;
  float paddings[kPushConstantFloatSize - 9];
};
#define CUSTOM_PUSH_TYPE PushConstant
#include "common.hlsli"
#include "sdf.hlsli"

[numthreads(8, 8, 1)] void main(uint3 id : SV_DispatchThreadID) {
  if(id.x >= push.width || id.y >= push.height)
    return;

  float2 uv = (id.xy + 0.5) * float2(push.invWidth, push.invHeight);
  float depth = sampleTexture2d(push.depthTextureIndex, push.sampler, uv);

  if(depth <= kEpsilon)
    return;

  float2 st = uv * 2 - 1.0;
}
