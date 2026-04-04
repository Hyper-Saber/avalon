#ifndef AVALON_NOISE_HLSLI
#define AVALON_NOISE_HLSLI

float hash11(float p) {
  p = frac(p * 0.1031);
  p *= p + 33.33;
  p *= p + p;
  return frac(p);
}

float hash21(float2 p) {
  float3 p3 = frac(float3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return frac((p3.x + p3.y) * p3.z);
}

float hash31(float3 p3) {
  p3 = frac(p3 * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return frac((p3.x + p3.y) * p3.z);
}

float noise1d(float p) {
  float i = floor(p);
  float f = frac(p);

  float a = hash11(i);
  float b = hash11(i + 1.0);
  float u = f * f * (3.0 - 2.0f);

  return lerp(a, b, u) * 2.0 - 1.0;
}

float noise2d(float2 p) {
  float2 i = floor(p);
  float2 f = frac(p);

  float a = hash21(i);
  float b = hash21(i + float2(1.0, 0.0));
  float c = hash21(i + float2(0.0, 1.0));
  float d = hash21(i + float2(1.0, 1.0));

  float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

  float res = lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
  return res * 2.0 - 1.0;
}

float fbm1d(float p, int octaves) {
  float value = 0.0;
  float amplitude = 0.5;
  float frequency = 1.0;

  for(int i = 0; i < octaves; i++) {
    value += amplitude * noise1d(p * frequency);
    frequency *= 2.0;
    amplitude *= 0.5;
  }

  return value;
}

float fbm2d(float2 p, int octaves) {
  float value = 0.0;
  float amplitude = 0.5;
  const float2x2 rot = float2x2(0.8, 0.6, -0.6, 0.8);

  for(int i = 0; i < octaves; i++) {
    value += amplitude * noise2d(p);
    p = mul(rot, p) * 2.02;
    amplitude *= 0.5;
  }
  return value;
}

float noise3d(float3 p) {
  float3 i = floor(p);
  float3 f = frac(p);

  float v000 = hash31(i + float3(0, 0, 0));
  float v100 = hash31(i + float3(1, 0, 0));
  float v010 = hash31(i + float3(0, 1, 0));
  float v110 = hash31(i + float3(1, 1, 0));

  float v001 = hash31(i + float3(0, 0, 1));
  float v101 = hash31(i + float3(1, 0, 1));
  float v011 = hash31(i + float3(0, 1, 1));
  float v111 = hash31(i + float3(1, 1, 1));

  float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

  float res =
      lerp(lerp(lerp(v000, v100, u.x), lerp(v010, v110, u.x), u.y),
           lerp(lerp(v001, v101, u.x), lerp(v011, v111, u.x), u.y), u.z);

  return res * 2.0 - 1.0;
}

float fbm3d(float3 p, int octaves) {
  float value = 0.0;
  float amplitude = 0.5;
  for(int i = 0; i < octaves; i++) {
    value += amplitude * noise3d(p);
    p = p * 2.02;
    amplitude *= 0.5;
  }
  return value;
}
#endif
