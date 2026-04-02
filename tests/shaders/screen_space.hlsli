struct VsOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 viewDir : TEXCOORD1;
};

VsOutput FullscreenBase(uint vertexID) {
  VsOutput output;

  static const float2 uvTable[3] = {float2(0, 0), float2(0, 2), float2(2, 0)};

  float2 uv = uvTable[vertexID];
  output.uv = uv;

  output.pos = float4(uv.x * 2.0f - 1.0f, uv.y * 2.0f - 1.0f, 0.5f, 1.0f);

  return output;
}

VsOutput FullscreenSkybox(uint vertexID, float4x4 invProjection,
                          float4x4 invView) {
  VsOutput output;

  static const float2 uvTable[3] = {float2(0, 0), float2(0, 2), float2(2, 0)};

  float2 uv = uvTable[vertexID];
  output.uv = uv;

  output.pos = float4(uv.x * 2.0f - 1.0f, uv.y * 2.0f - 1.0f, 1e-6, 1.0f);

  float4 viewPos = mul(invProjection, output.pos);
  float3 viewRay = viewPos.xyz / viewPos.w;

  output.viewDir = mul((float3x3)invView, viewRay);

  return output;
}
