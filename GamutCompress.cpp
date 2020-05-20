kernel GamutCompression : ImageComputationKernel<ePixelWise> {
  Image<eRead, eAccessPoint, eEdgeClamped> src;
  Image<eWrite> dst;

  param:
    float threshold;
    float cyan;
    float magenta;
    float yellow;
    bool invert;

  local:
  float thr;
  float3 lim;


  void init() {
    // thr is the percentage of the core gamut to protect: the complement of threshold.
    thr = 1 - threshold;
    
    // lim is the max distance from the gamut boundary that will be compressed
    // 0 is a no-op, 1 will compress colors from a distance of 2.0 from achromatic to the gamut boundary
    lim = float3(cyan+1, magenta+1, yellow+1);
  }


  // calc compressed distance
  float compress(float dist, float lim) {
    float cdist;
      if (dist < thr) {
        cdist = dist;
      } else {
        if (invert == 0) {
          cdist = thr + 1/(1/(dist - thr) + 1/(1 - thr) - 1/(lim - thr));
        } else {
          cdist = thr + 1/(1/(dist - thr) - 1/(1 - thr) + 1/(lim - thr));
        }
      }
    return cdist;
  }

  void process() {
    // source pixels
    float3 rgb = float3(src().x, src().y, src().z);

    // achromatic axis 
    float ach = max(rgb.x, max(rgb.y, rgb.z));

    // distance from the achromatic axis for each color component aka inverse rgb ratios
    float3 dist = ach == 0 ? float3(0, 0, 0) : fabs(rgb-ach)/ach;

    // compress distance with user controlled parameterized shaper function
    float3 cdist = float3(
      compress(dist.x, lim.x),
      compress(dist.y, lim.y),
      compress(dist.z, lim.z));

    // recalculate rgb from compressed distance and achromatic
    // effectively this scales each color component relative to achromatic axis by the compressed distance
    float3 crgb = ach-cdist*ach;

    // write output
    dst() = float4(crgb.x, crgb.y, crgb.z, src().w);
  }
};