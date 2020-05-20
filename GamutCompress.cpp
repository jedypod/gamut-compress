kernel GamutCompression : ImageComputationKernel<ePixelWise> {
  Image<eRead, eAccessPoint, eEdgeClamped> src;
  Image<eWrite> dst;

  param:
    float threshold;
    float cyan;
    float magenta;
    float yellow;
    int method; // compression method
    bool invert;

  local:
  float thr;
  float3 lim;
  float pi;

  void init() {
    pi = 3.14159265359;

    // thr is the percentage of the core gamut to protect: the complement of threshold.
    thr = (1 - threshold);
        
    // lim is the max distance from the gamut boundary that will be compressed
    // 0 is a no-op, 1 will compress colors from a distance of 2.0 from achromatic to the gamut boundary
    // if method is Reinhard, use the limit as-is
    if (method == 0) {
      lim = float3(cyan+1, magenta+1, yellow+1);
    } else {
      // otherwise, we have to bruteforce the value of limit 
      // such that lim is the value of x where y=1 - also enforce sane ranges to avoid nans
      // importantly, this runs once at the beginning of evaluation, NOT per-pixel!!!
      lim = float3(bisect(max(0.0001, cyan)+1), bisect(max(0.0001, magenta)+1), bisect(max(0.0001, yellow)+1));
    }
  }

  // calculate hyperbolic tangent
  float tanh( float in) {
    float f = exp(2.0f*in);
    return (f-1.0f) / (f+1.0f);
  }

  // calculate inverse hyperbolic tangent
  float atanh( float in) {
    return log((1.0f+in)/(1.0f-in))/2.0f;
  }

  // compression function which gives the y=1 x intersect at y=0
  float f(float x, float k) {
    if (method == 0) {
      return k;
    } else if (method == 1) {
      // natural exponent compression method
      return -log((-x+1)/(thr-x))*(-thr+x)+thr-k;
    } else if (method == 2) {
      // arctangent compression method
      return (2*tan( (pi*(1-thr))/(2*(x-thr)))*(x-thr))/pi+thr-k;
    } else if (method == 3) {
      // hyperbolic tangent compression method
      return atanh((1-thr)/(x-thr))*(x-thr)+thr-k;
    }
  }

  float bisect(float k) {
    // use a simple bisection algorithm to bruteforce the root of f
    // returns an approximation of the value of limit 
    // such that the compression function intersects y=1 at desired value k
    // this allows us to specify the max distance we will compress to the gamut boundary

    // initial guess
    float a = 1.001;
    float b = 100.0;

    // verify guess: this should always be true with the functions we have enabled
    bool good_guess = (f(a, k) < 0 && f(b, k) > 0) || (f(a, k) > 0 && f(b, k) < 0);
    if (good_guess != 1) {
      return 1;
    }

    float c, v;
    float tolerance = 0.0001;
    int nmax = 500; // max iterations
    int n = 0; // current iteration
    while (n <= nmax) {
      c = (a + b)/2; // midpoint between a and b
      v = f(c, k); // function value
      if (v == 0 || (b-a)/2 < tolerance) {
        break; // result found: c will be returned after while loop
      }
      if (v > 0 && f(a, k) > 0) {
        a = c; // c is new lower bound
      } else {
        b = c; // c is new upper bound
      }
      n += 1;
    }
    return c;
  }


  // calc compressed distance
  float3 compress(float3 dist) {
    float3 cdist;
    float cd;

    for (int i = 0; i < 3; i++) {
      if (dist[i] < thr) {
        cd = dist[i];
      } else {
        if (method == 0) {
          // simple Reinhard type compression suggested by Nick Shaw and Lars Borg
          // https://community.acescentral.com/t/simplistic-gamut-mapping-approaches-in-nuke/2679/3
          // https://community.acescentral.com/t/rgb-saturation-gamut-mapping-approach-and-a-comp-vfx-perspective/2715/52
          // example plot: https://www.desmos.com/calculator/h2n8smtgkl
          if (invert == 0) {
            cd = thr + 1/(1/(dist[i] - thr) + 1/(1 - thr) - 1/(lim[i] - thr));
          } else {
            cd = thr + 1/(1/(dist[i] - thr) - 1/(1 - thr) + 1/(lim[i] - thr));
          }
        } else if (method == 1) {
          // natural exponent compression method: plot https://www.desmos.com/calculator/jf99glamuc
          if (invert == 0) {
            cd = lim[i]-(lim[i]-thr)*exp(-(((dist[i]-thr)*((1*lim[i])/(lim[i]-thr))/lim[i])));
          } else {
            cd = -log((dist[i]-lim[i])/(thr-lim[i]))*(-thr+lim[i])/1+thr;
          }
        } else if (method == 2) {
          // arctangent compression method: plot https://www.desmos.com/calculator/olmjgev3sl
          if (invert == 0) {
            cd = thr + (lim[i] - thr) * 2 / pi * atan(pi/2 * (dist[i] - thr)/(lim[i] - thr));
          } else {
            cd = thr + (lim[i] - thr) * 2 / pi * tan(pi/2 * (dist[i] - thr)/(lim[i] - thr));
          }
        } else if (method == 3) {
          // hyperbolic tangent compression method: plot https://www.desmos.com/calculator/sapcakq6t1
          if (invert == 0) {
            cd = thr + (lim[i] - thr) * tanh( ( (dist[i]- thr)/( lim[i]-thr)));
          } else {
            cd = thr + (lim[i] - thr) * atanh( dist[i]/( lim[i] - thr) - thr/( lim[i] - thr));
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
    // distance is normalized by achromatic, so that 1.0 is at gamut boundary, and avoiding 0 div
    float3 dist = ach == 0 ? float3(0, 0, 0) : fabs(rgb-ach)/ach; 

    // compress distance with user controlled parameterized shaper function
    float3 cdist = compress(dist);

    // recalculate rgb from compressed distance and achromatic
    // effectively this scales each color component relative to achromatic axis by the compressed distance
    float3 crgb = ach-cdist*ach;

    // write to output
    dst() = float4(crgb.x, crgb.y, crgb.z, src().w);
  }
};