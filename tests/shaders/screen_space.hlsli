#ifndef AVALON_SCREEN_SPACE_HLSLI
#define AVALON_SCREEN_SPACE_HLSLI

struct VSOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 viewDir : TEXCOORD1;
};

struct FullscreenVertexAttribute {
  float4 pos;
  float2 uv;
};

FullscreenVertexAttribute generateVertices(uint vertexId) {
  FullscreenVertexAttribute output;
  static const float2 uvTable[3] = {float2(0, 0), float2(0, 2), float2(2, 0)};

  float2 uv = uvTable[vertexId];
  output.uv = uv;

  output.pos = float4(uv.x * 2.0 - 1.0, uv.y * 2.0 - 1.0, 1.0, 1.0);
  return output;
}

VSOutput fullscreenBase(uint vertexId) {
  VSOutput output;
  FullscreenVertexAttribute attribute = generateVertices(vertexId);

  output.uv = attribute.uv;
  output.pos = attribute.pos;

  return output;
}

VSOutput fullscreenSkybox(uint vertexId, float4x4 invProjection,
                          float4x4 invView) {
  VSOutput output;
  FullscreenVertexAttribute attribute = generateVertices(vertexId);

  output.uv = attribute.uv;
  output.pos = attribute.pos;

  float4 viewPos = mul(invProjection, output.pos);
  float3 viewRay = viewPos.xyz / viewPos.w;

  output.pos.z = 1e-6;
  output.viewDir = mul((float3x3)invView, viewRay);

  return output;
}
#endif
