#include "common.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
};

VSOutput VsMain(StandardVSInput input) {
  VSOutput output;
  float4 worldPos = mul(mModel, float4(input.position, 1.0));
  output.pos = calculateClipPosition(worldPos);
  return output;
}

void FsMain(VSOutput input) { return; }
