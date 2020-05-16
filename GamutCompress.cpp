kernel GamutCompression : ImageComputationKernel<ePixelWise> {
  Image<eRead, eAccessPoint, eEdgeClamped> src;
  Image<eWrite> dst;

  param:
    float threshold;
    float cyan;
    float magenta;
    float yellow;
    int method;
    bool invert;

  local:
  float thr;
  float3 lim;


  void init() {
    // thr is the percentage of the core gamut to protect: the complement of threshold.
    thr = 1.0f - threshold;

    // bias limits by color component
    // range is limited to 0.00001 > lim < 1/thr
    // cyan = 0: no compression
    // cyan = 1: "normal" compression with limit at 1.0
    // 1 > cyan < 1/thr : compress more than edge of gamut. max = hard clip (e.g., thr=0.8, max = 1.25)
    lim = float3(
      1.0f/max(0.00001f, min(1.0f/thr, cyan)), 
      1.0f/max(0.00001f, min(1.0f/thr, magenta)),
      1.0f/max(0.00001f, min(1.0f/thr, yellow))
      );
  }

  // calc hyperbolic tangent
  float tanh( float in) {
    float f = exp(2.0f*in);
    return (f-1.0f) / (f+1.0f);
  }

  // calc inverse hyperbolic tangent
  float atanh( float in) {
    return log((1.0f+in)/(1.0f-in))/2.0f;
  }

  // calc compressed distance
  float3 compress(float3 dist) {
    float3 cdist;
    float cd;

    // method 0 : tanh - hyperbolic tangent compression method suggested by Thomas Mansencal https://community.acescentral.com/t/simplistic-gamut-mapping-approaches-in-nuke/2679/2
    // method 1 : exp - natural exponent compression method
    // method 2 : simple - simple Reinhard type compression suggested by Nick Shaw and Lars Borg
      // https://community.acescentral.com/t/simplistic-gamut-mapping-approaches-in-nuke/2679/3
      // https://community.acescentral.com/t/rgb-saturation-gamut-mapping-approach-and-a-comp-vfx-perspective/2715/52
    // example plots for each method: https://www.desmos.com/calculator/x69iyptspq

    for (int i = 0; i < 3; i++) {
      if (dist[i] < thr) {
        cd = dist[i];
      } else {
        if (method == 0.0f) {
          if (invert == 0.0f) {
            cd = thr + (lim[i] - thr) * tanh( ( (dist[i] - thr)/( lim[i]-thr)));
          } else {
              cd = thr + (lim[i] - thr) * atanh( dist[i]/( lim[i] - thr) - thr/( lim[i] - thr));
          }
        } else if (method == 1.0f) {
          if (invert == 0.0f) {
            cd = lim[i]-(lim[i]-thr)*exp(-(((dist[i]-thr)*((1.0f*lim[i])/(lim[i]-thr))/lim[i])));
          } else {
            cd = -log( (dist[i]-lim[i])/(thr-lim[i]))*(-thr+lim[i])/1.0f+thr;
          }
        } else if (method == 2.0f) {
          if (invert == 0.0f) {
            cd = thr + 1/(1/(dist[i] - thr) + 1/(lim[i] - thr));
          } else {
            cd = thr + 1/(1/(dist[i] - thr) + -1/(lim[i] - thr));
          }
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