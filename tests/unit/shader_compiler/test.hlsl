struct VSInput {
[ [ vk : : location(0)] ] float3 position : POSITION;
[ [ vk : : location(1)] ] float2 uv : TEXCOORD;
} ;

struct VSOutput {
  float4 clipPos : SV_POSITION;
} ;

VSOutput Main(VSInput input) {
  VSOutput output;
  output.clipPos = float4(input.position, 1.);
  return output;
}
