// struct CameraUniform {
//   float4x4 view;
//   float4x4 projection;
//   float3 position;
//   float padding;
// };

// ConstantBuffer<CameraUniform> camera : register(b0);

struct VSInput {
  VK_LOCATION(0) float3 position : POSITION;
  VK_LOCATION(1) float2 uv : TEXCOORD;
  VK_LOCATION(2) float4 color : COLOR;
};

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float4 color : COLOR;
  VK_LOCATION(1) float2 uv : TEXCOORD;
};

VSOutput VsMain(VSInput input) {
  VSOutput output;
  // float4 worldPos = float4(input.position, 1.0f);
  // float4 viewPos = mul(camera.view, worldPos);
  // output.clipPos = mul(camera.projection, viewPos);
  output.color = input.color;
  output.uv = input.uv;
  output.clipPos = float4(input.position, 1.0);

  return output;
}

float4 FsMain(VSOutput input) : SV_TARGET { return input.color; }
