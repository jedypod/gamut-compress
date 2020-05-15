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

  // calc hyperbolic tangent
  float tanh( float in) {
    float f = exp(2.0f*in);
    return (f-1.0f) / (f+1.0f);
  }

  // calc inverse hyperbolic tangent
  float atanh( float in) {
    return log((1.0f+in)/(1.0f-in))/2.0f;
  }

  void process() {
    // thr is the complement of threshold. 
    // that is: the percentage of the core gamut to protect
    float thr = 1.0f - threshold;

    // bias limits by color component
    // range is limited to 0.00001 > lim < 1/thr
    // cyan = 0: no compression
    // cyan = 1: "normal" compression with limit at 1.0
    // 1 > cyan < 1/thr : compress more than edge of gamut. max = hard clip (e.g., thr=0.8, max = 1.25)
    float3 lim;
    lim.x = 1.0f/max(0.00001f, min(1.0f/thr, cyan));
    lim.y = 1.0f/max(0.00001f, min(1.0f/thr, magenta));
    lim.z = 1.0f/max(0.00001f, min(1.0f/thr, yellow));

    float r = src().x;
    float g = src().y;
    float b = src().z;

    // achromatic axis 
    float ach = max(r, max(g, b));

    // distance from the achromatic axis for each color component
    float d_r, d_g, d_b;
    d_r = ach == 0 ? 0 : fabs(float(r-ach)) / ach;
    d_g = ach == 0 ? 0 : fabs(float(g-ach)) / ach;
    d_b = ach == 0 ? 0 : fabs(float(b-ach)) / ach;

    // compress distance for each color component
    // method 0 : tanh - hyperbolic tangent compression method suggested by Thomas Mansencal https://community.acescentral.com/t/simplistic-gamut-mapping-approaches-in-nuke/2679/2
    // method 1 : exp - natural exponent compression method
    // method 2 : simple - simple Reinhard type compression suggested by Nick Shaw https://community.acescentral.com/t/simplistic-gamut-mapping-approaches-in-nuke/2679/3
    // example plots for each method: https://www.desmos.com/calculator/x69iyptspq
    float cd_r, cd_g, cd_b;
    if (method == 0.0f) {
      if (invert == 0.0f) {
        cd_r = d_r > thr ? thr + (lim.x - thr) * tanh( ( (d_r - thr)/( lim.x-thr))) : d_r;
        cd_g = d_g > thr ? thr + (lim.y - thr) * tanh( ( (d_g - thr)/( lim.y-thr))) : d_g;
        cd_b = d_b > thr ? thr + (lim.z - thr) * tanh( ( (d_b - thr)/( lim.z-thr))) : d_b;
      } else {
          cd_r = d_r > thr ? thr + (lim.x - thr) * atanh( d_r/( lim.x - thr) - thr/( lim.x - thr)) : d_r;
          cd_g = d_g > thr ? thr + (lim.y - thr) * atanh( d_g/( lim.y - thr) - thr/( lim.y - thr)) : d_g;
          cd_b = d_b > thr ? thr + (lim.z - thr) * atanh( d_b/( lim.z - thr) - thr/( lim.z - thr)) : d_b;
      }
    } else if (method == 1.0f) {
      if (invert == 0.0f) {
        cd_r = d_r > thr ? lim.x-(lim.x-thr)*exp(-(((d_r-thr)*((1.0f*lim.x)/(lim.x-thr))/lim.x))) : d_r;
        cd_g = d_g > thr ? lim.y-(lim.y-thr)*exp(-(((d_g-thr)*((1.0f*lim.y)/(lim.y-thr))/lim.y))) : d_g;
        cd_b = d_b > thr ? lim.z-(lim.z-thr)*exp(-(((d_b-thr)*((1.0f*lim.z)/(lim.z-thr))/lim.z))) : d_b;
      } else {
        cd_r = d_r > thr ? -log( (d_r-lim.x)/(thr-lim.x))*(-thr+lim.x)/1.0f+thr : d_r;
        cd_g = d_g > thr ? -log( (d_g-lim.y)/(thr-lim.y))*(-thr+lim.y)/1.0f+thr : d_g;
        cd_b = d_b > thr ? -log( (d_b-lim.z)/(thr-lim.z))*(-thr+lim.z)/1.0f+thr : d_b;
      }
    } else if (method == 2.0f) {
      if (invert == 0.0f) {
        cd_r = d_r > thr ? thr+(-1/((d_r-thr)/(lim.x-thr)+1)+1)*(lim.x-thr) : d_r;
        cd_g = d_g > thr ? thr+(-1/((d_g-thr)/(lim.y-thr)+1)+1)*(lim.y-thr) : d_g;
        cd_b = d_b > thr ? thr+(-1/((d_b-thr)/(lim.z-thr)+1)+1)*(lim.z-thr) : d_b;
      } else {
        cd_r = d_r > thr ? (pow(thr, 2.0f) - thr*d_r + (lim.x-thr)*d_r) / (thr + (lim.x-thr) - d_r) : d_r;
        cd_g = d_g > thr ? (pow(thr, 2.0f) - thr*d_g + (lim.y-thr)*d_g) / (thr + (lim.y-thr) - d_g) : d_g;
        cd_b = d_b > thr ? (pow(thr, 2.0f) - thr*d_b + (lim.z-thr)*d_b) / (thr + (lim.z-thr) - d_b) : d_b;
      }
    }

    // scale each color component relative to achromatic axis by gamut compression factor
    float c_r, c_g, c_b;
    c_r = ach-cd_r*ach;
    c_g = ach-cd_g*ach;
    c_b = ach-cd_b*ach;

    // write to output
    dst() = float4(c_r, c_g, c_b, 1);

  }
};