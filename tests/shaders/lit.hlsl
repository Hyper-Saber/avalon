#include "common.hlsli"

VK_PUSH_CONSTANT ModelData push;

struct VSInput {
  VK_LOCATION(0) float3 position : POSITION;
  VK_LOCATION(1) float3 color : COLOR;
  VK_LOCATION(2) float3 normal : NORMAL;
};

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float3 color : COLOR;
  VK_LOCATION(1) float3 worldPos : POSITION;
  VK_LOCATION(2) float3 worldNormal : NORMAL;
};

VSOutput VsMain(VSInput input) {
  VSOutput output;
  float4 worldPos = mul(push.model, float4(input.position, 1.0f));
  float4 viewPos = mul(uCamera.view, worldPos);
  output.clipPos = mul(uCamera.projection, viewPos);
  output.color = input.color;
  output.worldPos = worldPos.xyz;
  output.worldNormal = mul((float3x3)push.model, input.normal);

  return output;
}

float4 FsMain(VSOutput input) : SV_TARGET {
  float3 N = normalize(input.worldNormal);
  float3 V = normalize(uCamera.cameraPosition.xyz - input.worldPos);
  float3 L = normalize(-uMainLight.dirOrPos.xyz);

  float3 baseColor = input.color * float3(1, 0, 0);
  float3 lightColor = uMainLight.color.rgb * uMainLight.color.a;

  float3 ambient = 0.1 * baseColor;

  float diff = max(dot(N, L), 0.0);
  float3 diffuse = diff * lightColor * baseColor;

  float3 H = normalize(L + V);
  float spec = pow(max(dot(N, H), 0.0), 200.0);
  float3 specular = spec * lightColor;

  return float4(ambient + diffuse + specular, 1.0);
}
