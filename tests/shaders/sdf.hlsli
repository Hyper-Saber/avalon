#ifndef AVALON_SDF_HLSLI
#define AVALON_SDF_HLSLI

float sdfShpere(float3 p, float r) { return length(p) - r; }

float sdfBox(float3 p, floats s) {
  float3 q = abs(p) - s;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

#endif
