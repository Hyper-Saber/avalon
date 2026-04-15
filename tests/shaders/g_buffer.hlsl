#include "common.hlsli"

struct VSOutput {
  float4 pos : SV_POSITION;
  VK_LOCATION(0) float3 normal : NORMAL;
  VK_LOCATION(1) nointerpolation uint id : INSTANCE_ID;
};

struct FSOutput {
  float4 normal : SV_Target0;
  uint id : SV_Target1;
};

VSOutput VsMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOutput output;
  InstanceData d = loadInstanceData(instanceID);
  VertexGeometry geometry = loadVertexGeometry(vertexID, d.geometryOffset);
  float4 worldPos = calculateWorldPosition(d.modelOffset, geometry.position);
  float3 normal = calculateWorldNormal(d.invModelOffset, geometry.normal);
  output.pos = calculateClipPosition(worldPos);
  output.id = d.instanceID;
  output.normal = normal;
  return output;
}

FSOutput FsMain(VSOutput input) {
  FSOutput output;
  output.normal = float4(normalize(input.normal), 0.0);
  output.id = input.id;
  return output;
}
