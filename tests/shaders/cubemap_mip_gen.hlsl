#include "constants.hlsli"
struct FaceData {
  float4 forward;
  float4 right;
  float4 up;
};

struct CustomPush {
  uint faceDataOffset;
  uint srcIndex;
  uint dstIndex;
  uint samplerIndex;
  uint sampleCount;
  uint size;
  float invSize;
  float roughness;
  uint paddings[kPushConstantFloatSize - 8];
};

#define CUSTOM_PUSH_TYPE CustomPush
#include "common.hlsli"
#include "pbr.hlsli"

groupshared FaceData s_faces[6];

bool getLocalUV(float3 L, FaceData face, out float2 uv) {
  float3 f = face.forward.xyz;
  float3 r = face.right.xyz;
  float3 u = face.up.xyz;
  float d = dot(L, f);

  if(d <= 0.0001)
    return false;

  float3 projected = L / d;
  float x = dot(projected, r);
  float y = dot(projected, u);

  uv = float2(x, y) * 0.5 + 0.5;

  return (uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0);
}

float3 getUVL(float3 L, uint currentFace) {
  float2 localUV;

  if(getLocalUV(L, s_faces[currentFace], localUV)) {
    return float3(localUV, (float)currentFace);
  }

  float maxDot = -1.0;
  uint targetFace = currentFace;
  float2 globalUV = 0;
  [unroll] for(uint i = 0; i < 6; i++) {
    if(i == currentFace)
      continue;
    float3 f = s_faces[i].forward.xyz;
    float d = dot(L, f);
    if(d > maxDot) {
      maxDot = d;
      targetFace = i;

      float3 r = s_faces[i].right.xyz;
      float3 u = s_faces[i].up.xyz;
      float3 projected = L / d;
      globalUV = float2(dot(projected, r), dot(projected, u)) * 0.5 + 0.5;
    }
  }

  return float3(globalUV, (float)targetFace);
}

float3 getCubeMapDirection(float2 uv, uint faceIndex) {
  float2 temp = uv * 2.0 - 1.0;

  float3 f = s_faces[faceIndex].forward.xyz;
  float3 r = s_faces[faceIndex].right.xyz;
  float3 u = s_faces[faceIndex].up.xyz;

  float3 dir = f + (temp.x * r) + (temp.y * u);

  return normalize(dir);
}

[numthreads(8, 8, 1)] void CsMain(uint3 dispatchId : SV_DispatchThreadID,
                                  uint groupIndex : SV_GroupIndex) {
  uint size = push.size;

  if(dispatchId.x >= size || dispatchId.y >= size)
    return;

  if(groupIndex < 6) {
    uint offset = push.faceDataOffset + groupIndex * 48;
    s_faces[groupIndex] = uGeneralSSBO.Load<FaceData>(offset);
  }
  GroupMemoryBarrierWithGroupSync();

  float2 uv = float2(dispatchId.xy + 0.5) * push.invSize;
  float3 N = getCubeMapDirection(uv, dispatchId.z);
  float3 R = N;
  float3 V = R;

  float3 prefilteredColor = 0.0;
  float totalWeight = 0.0;

  uint srcIndex = push.srcIndex;
  uint dstIndex = push.dstIndex;
  uint samplerIndex = push.samplerIndex;
  uint sampleCount = push.sampleCount;
  float roughness = push.roughness;

  for(uint i = 0; i < sampleCount; i++) {
    float2 Xi = hammersley(i, sampleCount);
    float3 H = importanceSampleGGX(Xi, N, roughness);
    float3 L = normalize(2.0 * dot(V, H) * H - V);

    float NdotL = max(dot(N, L), 0.0);
    if(NdotL > 0.0) {
      prefilteredColor += sampleTextureArrayLod(srcIndex, samplerIndex,
                                                getUVL(L, dispatchId.z), 0.0)
                              .rgb *
                          NdotL;
      totalWeight += NdotL;
    }
  }

  if(totalWeight > 0.0) {
    prefilteredColor /= totalWeight;
  } else {
    prefilteredColor = sampleTextureArrayLod(srcIndex, samplerIndex,
                                             float3(uv, dispatchId.z), 0.0)
                           .rgb;
  }

  writeRWTextureArray(push.dstIndex, dispatchId, float4(prefilteredColor, 1.0));
}
