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
    thr = 1.0f - threshold;
    
    // lim is the max distance from the gamut boundary that will be compressed
    // 0 is a no-op, 1 will compress colors from a distance of 2.0 from achromatic to the gamut boundary
    lim = float3(cyan+1.0f, magenta+1.0f, yellow+1.0f);
  }


  // calc compressed distance
  float3 compress(float3 dist) {
    float3 cdist;
    float cd;

    // simple Reinhard type compression suggested by Nick Shaw and Lars Borg
      // https://community.acescentral.com/t/simplistic-gamut-mapping-approaches-in-nuke/2679/3
      // https://community.acescentral.com/t/rgb-saturation-gamut-mapping-approach-and-a-comp-vfx-perspective/2715/52
    // example plot: https://www.desmos.com/calculator/h2n8smtgkl

    for (int i = 0; i < 3; i++) {
      if (dist[i] < thr) {
        cd = dist[i];
      } else {
        if (invert == 0.0f) {
          cd = thr + 1/(1/(dist[i] - thr) + 1/(lim[i] - thr));
        } else {
          cd = thr + 1/(1/(dist[i] - thr) + -1/(lim[i] - thr));
        }
      }
      if (i==0){ cdist.x = cd; } else if (i==1) { cdist.y = cd;} else if (i==2) {cdist.z = cd;}
    }
    return cdist;
  }


  void process() {
    // source pixels
    float3 rgb = float3(src().x, src().y, src().z);

    // achromatic axis 
    float ach = max(rgb.x, max(rgb.y, rgb.z));

    // distance from the achromatic axis for each color component aka inverse rgb ratios
    float3 dist = ach == 0.0f ? float3(0.0f, 0.0f, 0.0f) : fabs(rgb-ach)/ach;

    // compress distance with user controlled parameterized shaper function
    float3 cd = compress(dist);

    // recalculate rgb from compressed distance and achromatic
    // effectively this scales each color component relative to achromatic axis by the compressed distance
    float3 c = ach-cd*ach;

    // write output
    dst() = float4(c.x, c.y, c.z, 1);
  }
};