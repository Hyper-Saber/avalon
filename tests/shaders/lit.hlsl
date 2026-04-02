#include "common.hlsli"

struct VSInput {
  VK_LOCATION(0) float3 position : POSITION;
  VK_LOCATION(1) float3 color : COLOR;
  VK_LOCATION(2) float2 uv : TEXCOORD0;
  VK_LOCATION(3) float3 normal : NORMAL;
};

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float3 color : COLOR;
  VK_LOCATION(1) float2 uv : TEXCOORD0;
  VK_LOCATION(2) float3 worldPos : POSITION;
  VK_LOCATION(3) float3 worldNormal : NORMAL;
};

VSOutput VsMain(VSInput input) {
  VSOutput output;

  float4 worldPos = mul(mModel, float4(input.position, 1.0f));
  output.clipPos = mul(uCamera.projection, mul(uCamera.view, worldPos));

  output.color = input.color;
  output.uv = input.uv;
  output.worldPos = worldPos.xyz;
  output.worldNormal = mul((float3x3)mNormalMatrix, input.normal);

  return output;
}

float4 FsMain(VSOutput input) : SV_Target {
  float3 N = normalize(input.worldNormal);
  float3 V = normalize(uCamera.cameraPosition.xyz - input.worldPos);
  float3 L = normalize(-uMainLight.posDir.xyz);

  // float4 texColor = SampleBindless(mMaterial.baseColorTexIdx,
  //                                  mMaterial.baseColorSamplerIdx, input.uv);
  // float3 baseColor = texColor.rgb * input.color * mMaterial.baseColor.rgb;
  float3 baseColor = input.color * mMaterial.baseColor.rgb;

  float3 lightColor =
      uMainLight.colorIntensity.rgb * uMainLight.colorIntensity.a;

  float3 ambient = float3(0.01, 0.01, 0.05) * baseColor;

  float3 H = normalize(L + V);

  float3 F0 = float3(mMaterial.f0, mMaterial.f0, mMaterial.f0);
  float3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(V, H), 0.0), 5.0);

  float3 kD = (1.0 - F);

  float dotNL = max(dot(N, L), 0.0);
  float3 diffuse = kD * dotNL * lightColor * baseColor / kPi;

  float shininess = max(mMaterial.shininess, 1.0);
  float energyConservation = (shininess + 8.0) / (8.0 * kPi);
  float spec = pow(max(dot(N, H), 0.0), shininess) * energyConservation;
  float3 specular = spec * F * lightColor * mMaterial.specularColor.rgb;

  return float4(ambient + diffuse + specular, 1);
  // texColor.a * mMaterial.baseColor.a);
}
