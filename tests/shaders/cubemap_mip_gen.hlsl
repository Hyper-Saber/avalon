#include "constants.hlsli"
struct CustomPush {
  uint srcIndex;
  uint dstIndex;
  uint samplerIndex;
  uint targetWidth;
  uint targetHeight;
  float invSize;
  uint paddings[kMaxCustomSlots - 6];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID) {
  float2 uv = float2(dispatchId.xy + 0.5) * push.custom.invSize;

  if(dispatchId.x >= push.custom.targetWidth ||
     dispatchId.y >= push.custom.targetHeight) {
    return;
  }

  float4 color =
      sampleTextureArrayLod(push.custom.srcIndex, push.custom.samplerIndex,
                            float3(uv, (float)dispatchId.z), 0.0);

  writeRWTextureArray(push.custom.dstIndex, dispatchId, color);
}
