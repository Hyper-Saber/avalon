struct VSInput {
  VK_LOCATION(0) float3 position : POSITION;
  VK_LOCATION(1) float2 uv : TEXCOORD;
};

struct VSOutput {
  float4 clipPos : SV_POSITION;
};

VSOutput Main(VSInput input) {
  VSOutput output;
  output.clipPos = float4(input.position, 1.);
  return output;
}
