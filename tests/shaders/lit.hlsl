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
                              uint prefilterMapIndex, uint brdfLutIndex,
                              uint samplerIndex) {
  float3 N = surface.normal;
  float3 V = viewDir;
  float3 R = reflect(-V, N);
  float NdotV = max(dot(N, V), 0.0);

  float3 F0 = lerp(float3(0.04, 0.04, 0.04), surface.albedo, surface.metallic);

  float3 envBRDF = sampleTexture2d(brdfLutIndex, samplerIndex,
                                   float2(NdotV, surface.roughness))
                       .rgb;

  float3 prefilteredColor =
      sampleCubeLod(prefilterMapIndex, samplerIndex, R, surface.roughness * 7.0)
          .rgb;

  float3 f_ss = F0 * envBRDF.x + envBRDF.y;

  float E_v = envBRDF.x + envBRDF.y;
  float E_avg = envBRDF.z;

  float3 F_avg = F0;

  float3 f_ms = (1.0 - E_v);
  float3 s_ms = F_avg * E_avg / (1.0 - F_avg * (1.0 - E_avg) + 1e6);
  float3 multiScatteringOrder = s_ms * f_ms;

  float specularOcclusion =
      saturate(pow(NdotV + surface.ao, 0.5) - 1.0 + surface.ao);
  specularOcclusion = 1;
  float3 specular =
      prefilteredColor * (f_ss + multiScatteringOrder) * specularOcclusion;

  float3 kS = f_ss + multiScatteringOrder;
  float3 kD = (1.0 - kS) * (1.0 - surface.metallic);

  CubemapSH sh = uDynamicSSBO.Load<CubemapSH>(push.skyboxSHOffset);
  float3 irradiance = evaluateSH(N, sh);
  float3 diffuse = irradiance * surface.albedo * surface.ao;

  return (kD * diffuse + specular);
}

float3 calculateDirectLight(PBRSurface surface, Light mainLight, float3 viewDir,
                            uint brdfLutIndex, uint samplerIndex) {
  float3 N = surface.normal;
  float3 V = viewDir;
  float3 L = normalize(-mainLight.posDir.xyz);
  float3 H = normalize(V + L);
  float NdotL = max(dot(N, L), 0.0);
  float NdotV = max(dot(N, V), 0.0);

  float3 F0 = lerp(float3(0.04, 0.04, 0.04), surface.albedo, surface.metallic);

  float D = distributionGGX(N, H, surface.roughness);
  float G_Vis = visibilitySmithJointGGX(NdotV, NdotL, surface.roughness);
  float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
  float3 specularSS = D * F * G_Vis;

  float3 brdfV = sampleTexture2d(brdfLutIndex, samplerIndex,
                                 float2(NdotV, surface.roughness))
                     .rgb;
  float3 brdfL = sampleTexture2d(brdfLutIndex, samplerIndex,
                                 float2(NdotL, surface.roughness))
                     .rgb;

  float E_v = brdfV.x + brdfV.y;
  float E_l = brdfL.x + brdfL.y;
  float E_avg = brdfV.z;

  float3 F_avg = F0;
  float3 s_ms = F_avg * E_avg / (1.0 - F_avg * (1.0 - E_avg) + 1e6);
  float3 specularMS =
      s_ms * (1.0 - E_v) * (1.0 - E_l) / (kPi * (1.0 - E_avg) + 1e6);

  float3 kS = (F0 * brdfV.x + brdfV.y) + (s_ms * (1.0 - E_v));
  float3 kd = (1.0 - saturate(kS)) * (1.0 - surface.metallic);

  float3 lightColor = mainLight.colorIntensity.rgb * mainLight.colorIntensity.a;
  return (kd * surface.albedo / kPi + specularSS + specularMS) * NdotL *
         lightColor;
}

struct VSOutput {
  float4 clipPos : SV_POSITION;
  VK_LOCATION(0) float3 color : COLOR;
  VK_LOCATION(1) float2 uv : TEXCOORD0;
  VK_LOCATION(2) float3 worldPos : POSITION;
  VK_LOCATION(3) float3 worldNormal : NORMAL;
  VK_LOCATION(4) nointerpolation uint materialID : MATERIAL_ID;
};

VSOutput VsMain(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID) {
  VSOutput output;

  InstanceData data = loadInstanceData(instanceID);
  VertexGeometry geometry = loadVertexGeometry(vertexID, data.geometryOffset);
  VertexAttributes attributes =
      loadVertexAttributes(vertexID, data.attributesOffset);

  float4 worldPos = calculateWorldPosition(data.modelOffset, geometry.position);
  output.worldPos = worldPos.xyz;
  output.clipPos = calculateClipPosition(worldPos);

  output.color = attributes.color;

  output.uv = geometry.uv;
  output.worldNormal =
      calculateWorldNormal(data.invModelOffset, geometry.normal);
  output.materialID = data.materialID;

  return output;
}

#define mMaterial uMaterials[materialID]

float4 FsMain(VSOutput input) : SV_Target {
  uint materialID = input.materialID;
  PBRSurface surface;
  surface.albedo = mMaterial.albedo.rgb;
  surface.metallic = mMaterial.metallic;
  surface.roughness = mMaterial.roughness;
  surface.normal = normalize(input.worldNormal);
  surface.ao = mMaterial.ao;
  surface.roughness = max(surface.roughness, 0.05);

  float4 shadow = loadTexture2d(push.shadowMask, uint2(input.clipPos.xy));
  surface.ao = min(surface.ao, shadow.g);

  float3 V = normalize(uCamera.worldPosition.xyz - input.worldPos);
  float3 direct = calculateDirectLight(surface, uMainLight, V, push.brdfLut,
                                       mMaterial.sampler);

  float3 indirect = calculateIndirectLight(surface, V, push.prefilterMap,
                                           push.brdfLut, mMaterial.sampler);

  float3 finalColor = direct * shadow.x + indirect;

  return float4(finalColor, 1.0);
}
