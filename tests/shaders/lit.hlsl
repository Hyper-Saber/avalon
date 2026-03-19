#include "common.hlsli"

VK_PUSH_CONSTANT ModelData push;

struct MaterialData {
  float4 baseColor;
  float4 specularColor;
  float shininess;
  float f0;
  float2 padding;
};

VK_BINDING(0, 1) ConstantBuffer<MaterialData> mMaterial;

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
  output.worldNormal = mul((float3x3)push.normalMatrix, input.normal);

  return output;
}

float4 FsMain(VSOutput input) : SV_TARGET {
  float3 N = normalize(input.worldNormal);
  float3 V = normalize(uCamera.cameraPosition.xyz - input.worldPos);
  float3 L = normalize(-uMainLight.dirOrPos.xyz);

  float3 baseColor = input.color * mMaterial.baseColor.rgb;
  float3 lightColor = uMainLight.color.rgb * uMainLight.color.a;

  float3 ambient = float3(0, 0, 0.1) * baseColor;

  float3 H = normalize(L + V);

  float3 F0 = mMaterial.f0;
  float3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(V, H), 0), 5.0);

  float3 kD = 1.0 - F;

  float diff = dot(N, L) * 0.5 + 0.5;
  float3 diffuse = kD * diff * lightColor * baseColor;

  float shininess = max(mMaterial.shininess, 1);
  float energyConservation = (shininess + 8.) / (8. * PI);
  float spec = pow(max(dot(N, H), 0.0), shininess) * energyConservation;
  float3 specular = spec * F * lightColor * mMaterial.specularColor.rgb;
  float dotNl = dot(N, L);
  float3 c = spec;
  return float4(ambient + diffuse + specular, 1.0);
}
