struct VsOutput {
  float4 pos : SV_POSITION;
  float2 uv : TEXCOORD0;
  float3 viewDir : TEXCOORD1;
};

VsOutput FullscreenBase(uint vertexID) {
  VsOutput output;
  output.uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.pos = float4(output.uv * 2 - 1, 1e-6, 1);
  output.pos.y = -output.pos.y;
  return output;
}

VsOutput FullscreenSkybox(uint vertexID, float4x4 invProjection,
                          float4x4 invView) {
  VsOutput output;

  float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
  output.uv = uv;
  output.pos = float4(uv * 2.0f - 1.0f, 1e-6, 1.0f);
  output.pos.y = -output.pos.y;

  float4 viewPos = mul(invProjection, output.pos);
  float3 viewRay = viewPos.xyz / viewPos.w;

  output.viewDir = mul((float3x3)invView, viewRay);

  return output;
}
