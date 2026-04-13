#include "common.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
};

VSOutput VsMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOutput output;
  InstanceData d = loadInstanceData(instanceID);
  VertexPosUV posuv = loadVertexPosUV(vertexID, d.posUVOffset);
  float4 worldPos = calculateWorldPosition(d.modelOffset, posuv.position);
  output.pos = calculateClipPosition(worldPos);
  return output;
}

void FsMain(VSOutput input) { return; }
