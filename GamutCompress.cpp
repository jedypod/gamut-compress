kernel GamutCompression : public ImageComputationKernel<ePixelWise> {
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
  float pi;

  void init() {
    pi = 3.14159265359;

    // thr is the percentage of the core gamut to protect: the complement of threshold.
    thr = (1 - threshold);
        
    // lim is the max distance from the gamut boundary that will be compressed
    // 0 is a no-op, 1 will compress colors from a distance of 2.0 from achromatic to the gamut boundary
    // if method is Reinhard, use the limit as-is
    if (method == 1) {
      lim = float3(cyan+1, magenta+1, yellow+1);
    } else {
      // otherwise, we have to bruteforce the value of limit 
      // such that lim is the value of x where y=1
      // importantly, this runs once at the beginning of evaluation, NOT per-pixel!!!
      lim = float3(
        bisect(max(0.0001, cyan)+1),
        bisect(max(0.0001, magenta)+1),
        bisect(max(0.0001, yellow)+1));
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
      // natural logarithm compression method
      return (exp((1-thr+thr*log(1-x)-x*thr*log(1-x))/(thr*(1-x))))*thr+x*thr-k;
    } else if (method == 1) {
      return k;
    } else if (method == 2) { 
      // natural exponent compression method
      return -log((-x+1)/(thr-x))*(-thr+x)+thr-k;
    } else if (method == 3) {
      // arctangent compression method
      return (2*tan( (pi*(1-thr))/(2*(x-thr)))*(x-thr))/pi+thr-k;
    } else if (method == 4) {
      // hyperbolic tangent compression method
      return atanh((1-thr)/(x-thr))*(x-thr)+thr-k;
    }
  }

  int sign(float x) {
    return x == 0 ? 0 : x > 0 ? 1 : 0;
  }

  float bisect(float k) {
    // use a simple bisection algorithm to bruteforce the root of f
    // returns an approximation of the value of limit 
    // such that the compression function intersects y=1 at desired value k
    // this allows us to specify the max distance we will compress to the gamut boundary
    
    float a, b, c, y;
    float tol = 0.0001; // accuracy of estimate
    int nmax = 100; // max iterations

    // set up reasonable initial guesses for each method given output ranges of each function
    if (method == 0) {
      // natural logarithm needs a limit between -inf (linear), and 1 (clip)
      a = -5;
      b = 0.96;
    } else if (method == 4) {
      // tanh needs more precision
      a = 1.000001;
      b = 5;
    } else {
      a = 1.0001;
      b = 5;
    }

    if (sign(f(a, k)) == sign(f(b, k))) {
      // bad estimate. return something close to linear
      if (method == 2) {
        return -100;
      } else {
        return 1.999999;
      }
    }
    c = (a+b)/2;
    y = f(c, k);
    if (y == 0) {
      return c; // lucky guess
    }

    int n = 1;
    while ((fabs(y) > tol) && (n <= nmax)) {
      if (sign(y) == sign(f(a, k))) {
        a = c;
      } else {
        b = c;
      }
      c = (a+b)/2;
      y = f(c, k);
      n += 1;
    }
    return c;
  }


  // calculate compressed distance
  float compress(float dist, float lim) {
    float cdist;
    if (dist < thr) {
      cdist = dist;
    } else {
      if (method == 0) {
        // natural logarithm compression method: https://www.desmos.com/calculator/hmzirlw7tj
        // inspired by ITU-R BT.2446 http://www.itu.int/pub/R-REP-BT.2446-2019
        if (invert == 0) {
          cdist = thr*log(dist/thr-lim)-lim*thr*log(dist/thr-lim)+thr-thr*log(1-lim)+lim*thr*log(1-lim);
        } else {
          cdist = exp((dist-thr+thr*log(1-lim)-lim*thr*log(1-lim))/(thr*(1-lim)))*thr+lim*thr;
        }
      } else if (method == 1) {
        // simple Reinhard type compression method: https://www.desmos.com/calculator/lkhdtjbodx
        if (invert == 0) {
          cdist = thr + 1/(1/(dist - thr) + 1/(1 - thr) - 1/(lim - thr));
        } else {
          cdist = thr + 1/(1/(dist - thr) - 1/(1 - thr) + 1/(lim - thr));
        }
      } else if (method == 2) {
        // natural exponent compression method: https://www.desmos.com/calculator/s2adnicmmr
        if (invert == 0) {
          cdist = lim-(lim-thr)*exp(-(((dist-thr)*((1*lim)/(lim-thr))/lim)));
        } else {
          cdist = -log((dist-lim)/(thr-lim))*(-thr+lim)/1+thr;
        }
      } else if (method == 3) {
        // arctangent compression method: https://www.desmos.com/calculator/h96qmnozpo
        if (invert == 0) {
          cdist = thr + (lim - thr) * 2 / pi * atan(pi/2 * (dist - thr)/(lim - thr));
        } else {
          cdist = thr + (lim - thr) * 2 / pi * tan(pi/2 * (dist - thr)/(lim - thr));
        }
      } else if (method == 4) {
        // hyperbolic tangent compression method: https://www.desmos.com/calculator/xiwliws24x
        if (invert == 0) {
          cdist = thr + (lim - thr) * tanh( ( (dist- thr)/( lim-thr)));
        } else {
          cdist = thr + (lim - thr) * atanh( dist/( lim - thr) - thr/( lim - thr));
        }
      }
    }
    return cdist;
  }


  void process() {
    // source pixels
    float3 rgb = float3(src()[0], src()[1], src()[2]);

    // achromatic axis 
    float ach = max(rgb[0], max(rgb[1], rgb[2]));

    // distance from the achromatic axis for each color component aka inverse rgb ratios
    // distance is normalized by achromatic, so that 1.0 is at gamut boundary, avoid 0 div
    float3 dist = ach == 0 ? float3(0, 0, 0) : (ach-rgb)/ach;

    // compress distance with user controlled parameterized shaper function
    float3 cdist = float3(
      compress(dist[0], lim[0]),
      compress(dist[1], lim[1]),
      compress(dist[2], lim[2]));

    // recalculate rgb from compressed distance and achromatic
    // effectively this scales each color component relative to achromatic axis by the compressed distance
    float3 crgb = ach-cdist*ach;

    // write to output
    dst() = float4(crgb[0], crgb[1], crgb[2], src()[3]);
  }
};