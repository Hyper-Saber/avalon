#include "common.hlsli"
#include "pbr.hlsli"

struct PBRSurface {
  float3 albedo;
  float metallic;
  float roughness;
  float3 normal;
  float ao;
};

float3 calculateIndirectLight(PBRSurface surface, float3 viewDir,
                              uint irradianceMapIndex, uint prefilterMapIndex,
                              uint brdfLutIndex, uint samplerIndex) {
  float3 N = surface.normal;
  float3 V = viewDir;
  float3 R = reflect(-V, N);
  float NdotV = max(dot(N, V), 0.0);

  float3 F0 = lerp(float3(0.04, 0.04, 0.04), surface.albedo, surface.metallic);
  float3 F = fresnelSchlickRoughness(NdotV, F0, surface.roughness);

  float3 kS = F;
  float3 kD = (1.0 - kS) * (1.0 - surface.metallic);
  // float3 irradiance = sampleCube(irradianceMapIndex, samplerIndex, N).rgb;
  // float3 diffuse = irradiance * surface.albedo;
  //
  const float MAX_REFLECTION_LOD = 7.0;

  float3 prefilteredColor =
      sampleCubeLod(prefilterMapIndex, samplerIndex, R,
                    surface.roughness * MAX_REFLECTION_LOD)
          .rgb;
  float2 envBRDF = sampleTexture2d(brdfLutIndex, samplerIndex,
                                   float2(NdotV, surface.roughness))
                       .rg;
  float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

  return specular;

  // return (kD * diffuse + specular) * surface.ao;
}

float3 calculateDirectLight(PBRSurface surface, Light mainLight,
                            float3 viewDir) {
  float3 N = surface.normal;
  float3 V = viewDir;
  float3 L = normalize(-mainLight.posDir.xyz);
  float3 H = normalize(V + L);
  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);

  float3 F0 = float3(0.04, 0.04, 0.04);
  F0 = lerp(F0, surface.albedo, surface.metallic);

  float D = distributionGGX(N, H, surface.roughness);
  float G_Vis = visibilitySmithJoint(NdotV, NdotL, surface.roughness);
  float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

  float3 specular = D * F * G_Vis;

  float3 ks = F;
  float3 kd = 1.0 - ks;
  kd *= 1.0 - surface.metallic;

  return (kd * surface.albedo / kPi + specular) * NdotL *
         mainLight.colorIntensity.rgb * mainLight.colorIntensity.a;
}

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
  PBRSurface surface;
  surface.albedo = mMaterial.albedo.rgb;
  surface.metallic = mMaterial.metallic;
  surface.roughness = mMaterial.roughness;
  surface.normal = normalize(input.worldNormal);
  surface.ao = mMaterial.ao;
  surface.roughness = max(surface.roughness, 0.05);
  float3 V = normalize(uCamera.worldPosition.xyz - input.worldPos);
  float3 direct = calculateDirectLight(surface, uMainLight, V);

  float3 indirect =
      calculateIndirectLight(surface, V, push.irradianceMap, push.prefilterMap,
                             push.brdfLut, mMaterial.sampler);

  // float3 c = surface.normal;
  // return float4(c, 1.0);
  // return float4(indirect, 1.0);
  return float4(direct + indirect, 1.0);
}
