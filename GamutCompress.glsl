uniform sampler2D frontTex, matteTex, selectiveTex;
uniform float power, cyan, magenta, yellow, shd_rolloff, adsk_result_w, adsk_result_h;
uniform int method, working_colorspace;
uniform bool invert, hexagonal;
uniform vec3 threshold;

const float pi = 3.14159265359;

// calc hyperbolic tangent
float tanh( float val) {
  float f = exp(2.0*val);
  return (f-1.0) / (f+1.0);
}

// calc inverse hyperbolic tangent
float atanh( float val) {
  return log((1.0+val)/(1.0-val))/2.0;
}

// Convert acescg to acescct
float lin_to_acescct(float val) {
  if (val <= 0.0078125) {
    return 10.5402377416545 * val + 0.0729055341958355;
  } else {
    return (log2(val) + 9.72) / 17.52;
  }
}

// Convert acescct to acescg
float acescct_to_lin(float val) {
  if (val > 0.155251141552511) {
    return pow( 2.0, val*17.52 - 9.72);
  } else {
    return (val - 0.0729055341958355) / 10.5402377416545;
  }
}

// Convert acescg to acescc
float lin_to_acescc(float val) {
  if (val <= 0.0) {
    return -0.3584474886; // =(log2( pow(2.,-16.))+9.72)/17.52
  } else if (val < pow(2.0, -15.0)) {
    return (log2(pow(2.0, -16.0)+val*0.5)+9.72)/17.52;
  } else { // (val >= pow(2.,-15))
    return (log2(val)+9.72)/17.52;
  }
}

// Convert acescc to acescg
float acescc_to_lin(float val) {
  if (val < -0.3013698630) { // (9.72-15)/17.52
    return (pow(2.0, val*17.52-9.72) - pow(2.0, -16.0))*2.0;
  } else if (val < (log2(65504.0)+9.72)/17.52) {
    return pow(2.0, val*17.52-9.72);
  } else { // (val >= (log2(HALF_MAX)+9.72)/17.52)
    return 65504.0;
  }
}

// compression function which gives the y=1 x in tersect at y=0
float f(float x, float k, float thr, int method) {
  if (method == 0) {
    // natural logarithm compression method
    return (exp((1.0-thr+thr*log(1.0-x)-x*thr*log(1.0-x))/(thr*(1.0-x))))*thr+x*thr-k;
  } else if (method == 1 || method == 2) {
    return k;
  } else if (method == 3) {
    // natural exponent compression method
    return -log((-x+1.0)/(thr-x))*(-thr+x)+thr-k;
  } else if (method == 4) {
    // arctangent compression method
    return (2.0*tan( (pi*(1.0-thr))/(2.0*(x-thr)))*(x-thr))/pi+thr-k;
  } else if (method == 5) {
    // hyperbolic tangent compression method
    return atanh((1.0-thr)/(x-thr))*(x-thr)+thr-k;
  }
}

int _sign(float x) {
  return x == 0.0 ? 0 : x > 0.0 ? 1 : 0;
}

float bisect(float k, float thr, int method) {
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
    a = -15.0;
    b = 0.98;
  } else if (method == 5) {
    // tanh needs more precision
    a = 1.000001;
    b = 5.0;
  } else {
    a = 1.0001;
    b = 5.0;
  }

  if (_sign(f(a, k, thr, method)) == _sign(f(b, k, thr, method))) {
    // bad estimate. return something close to linear
    if ((method == 0) || (method == 3)) {
      return -100.0;
    } else {
      return 1.999999;
    }
  }
  c = (a+b)/2.0;
  y = f(c, k, thr, method);
  if (abs(y) <= tol) {
    return c; // lucky guess
  }
  int n = 1;
  while ((abs(y) > tol) && (n <= nmax)) {
    if (_sign(y) == _sign(f(a, k, thr, method))) {
      a = c;
    } else {
      b = c;
    }
    c = (a+b)/2.0;
    y = f(c, k, thr, method);
    n += 1;
  }
  return c;
}

// calculate compressed distance
float compress(float dist, float lim, float thr, bool invert, int method, float power) {
  float cdist, s;
  if (dist < thr) {
    cdist = dist;
  } else {
    if (method == 0) {
      // natural logarithm compression method: https://www.desmos.com/calculator/hmzirlw7tj
      // inspired by ITU-R BT.2446 http://www.itu.int/pub/R-REP-BT.2446-2019
      if (!invert) {
        cdist = thr*log(dist/thr-lim)-lim*thr*log(dist/thr-lim)+thr-thr*log(1.0-lim)+lim*thr*log(1.0-lim);
      } else {
        cdist = exp((dist-thr+thr*log(1.0-lim)-lim*thr*log(1.0-lim))/(thr*(1.0-lim)))*thr+lim*thr;
      }
    } else if (method == 1) {
      // simple Reinhard type compression method: https://www.desmos.com/calculator/lkhdtjbodx
      if (!invert) {
        cdist = thr + 1.0/(1.0/(dist - thr) + 1.0/(1.0 - thr) - 1.0/(lim - thr));
      } else {
        cdist = thr + 1.0/(1.0/(dist - thr) - 1.0/(1.0 - thr) + 1.0/(lim - thr));
      }
    } else if (method == 2) {
      // power(p) compression function plot https://www.desmos.com/calculator/54aytu7hek
      if (lim < 1.0001) {
        return dist; // disable compression, avoid nan
      }
      s = (lim-thr)/pow(pow((1.0-thr)/(lim-thr),-power)-1.0,1.0/power); // calc y=1 intersect
      if (!invert) {
        cdist = thr+s*((dist-thr)/s)/(pow(1.0+pow((dist-thr)/s,power),1.0/power)); // compress
      } else {
        if (dist > (thr + s)) {
          cdist = dist; // avoid singularity
        }
        cdist = thr+s*pow(-(pow((dist-thr)/s,power)/(pow((dist-thr)/s,power)-1.0)),1.0/power); // uncompress
      }
    } else if (method == 3) {
      // natural exponent compression method: https://www.desmos.com/calculator/s2adnicmmr
      if (!invert) {
        cdist = lim-(lim-thr)*exp(-(((dist-thr)*((1.0*lim)/(lim-thr))/lim)));
      } else {
        cdist = -log((dist-lim)/(thr-lim))*(-thr+lim)/1.0+thr;
      }
    } else if (method == 4) {
      // arctangent compression method: plot https://www.desmos.com/calculator/olmjgev3sl
      if (!invert) {
        cdist = thr + (lim - thr) * 2.0 / pi * atan(pi/2.0 * (dist - thr)/(lim - thr));
      } else {
        cdist = thr + (lim - thr) * 2.0 / pi * tan(pi/2.0 * (dist - thr)/(lim - thr));
      }
    } else if (method == 5) {
      // hyperbolic tangent compression method: https://www.desmos.com/calculator/xiwliws24x
      if (!invert) {
        cdist = thr + (lim - thr) * tanh( ( (dist - thr)/( lim - thr)));
      } else {
        cdist = thr + (lim - thr) * atanh( dist/( lim - thr) - thr/( lim - thr));
      }
    }
  }
  return cdist;
}

void main() {
  vec2 coords = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
  // source pixels
  vec3 rgb = texture2D(frontTex, coords).rgb;
  float alpha = texture2D(matteTex, coords).g;
  float select = texture2D(selectiveTex, coords).g;

  if (working_colorspace == 1) {
    rgb.x = acescct_to_lin(rgb.x);
    rgb.y = acescct_to_lin(rgb.y);
    rgb.z = acescct_to_lin(rgb.z);
  } else if (working_colorspace == 2) {
    rgb.x = acescc_to_lin(rgb.x);
    rgb.y = acescc_to_lin(rgb.y);
    rgb.z = acescc_to_lin(rgb.z);
  } 

  // thr is the percentage of the core gamut to protect: the complement of threshold.
  vec3 thr = vec3(
    1.0-max(0.00001, threshold.x),
    1.0-max(0.00001, threshold.y),
    1.0-max(0.00001, threshold.z));

  // lim is the max distance from the gamut boundary that will be compressed
  // 0 is a no-op, 1 will compress colors from a distance of 2.0 from achromatic to the gamut boundary
  // if method is Reinhard, use the limit as-is
  vec3 lim;
  if (method == 1 || method == 2) {
    lim = vec3(cyan+1.0, magenta+1.0, yellow+1.0);
  } else {
    // otherwise, we have to bruteforce the value of limit 
    // such that lim is the value of x where y=1 - also enforce sane ranges to avoid nans

    // Not sure of a way to pre-calculate a constant using the values from the ui parameters in GLSL...
    // This approach might have performance implications
    lim = vec3(
      bisect(max(0.0001, cyan)+1.0, thr.x, method),
      bisect(max(0.0001, magenta)+1.0, thr.y, method),
      bisect(max(0.0001, yellow)+1.0, thr.z, method));
  }

  // achromatic axis 
  float ach = max(rgb.x, max(rgb.y, rgb.z));

  // achromatic with shadow rolloff below shd_rolloff threshold
  float ach_shd = 1.0-((1.0-ach)<(1.0-shd_rolloff)?(1.0-ach):(1.0-shd_rolloff)+shd_rolloff*tanh((((1.0-ach)-(1.0-shd_rolloff))/shd_rolloff)));

  // distance from the achromatic axis for each color component aka inverse rgb ratios
  vec3 dist;
  dist.x = ach_shd == 0.0 ? 0.0 : (ach-rgb.x)/ach_shd;
  dist.y = ach_shd == 0.0 ? 0.0 : (ach-rgb.y)/ach_shd;
  dist.z = ach_shd == 0.0 ? 0.0 : (ach-rgb.z)/ach_shd;

  // compress distance with user controlled parameterized shaper function
  float sat;
  vec3 csat, cdist;
  if (hexagonal) {
    // Based on Nick Shaw's variation on the gamut mapping algorithm 
    // https://community.acescentral.com/t/a-variation-on-jeds-rgb-gamut-mapper/3060
    sat = max(dist.x, max(dist.y, dist.z));
    csat = vec3(
      compress(sat, lim.x, thr.x, invert, method, power),
      compress(sat, lim.y, thr.y, invert, method, power),
      compress(sat, lim.z, thr.z, invert, method, power));
    cdist = sat == 0.0 ? dist : vec3(
      dist.x * csat.x / sat,
      dist.y * csat.y / sat,
      dist.z * csat.z / sat);
  } else {
    cdist = vec3(
      compress(dist.x, lim.x, thr.x, invert, method, power),
      compress(dist.y, lim.y, thr.y, invert, method, power),
      compress(dist.z, lim.z, thr.z, invert, method, power));
  }

  // recalculate rgb from compressed distance and achromatic
  // effectively this scales each color component relative to achromatic axis by the compressed distance
  vec3 crgb = vec3(
    ach-cdist.x*ach_shd,
    ach-cdist.y*ach_shd,
    ach-cdist.z*ach_shd);

  if (working_colorspace == 1) {
    crgb.x = lin_to_acescct(crgb.x);
    crgb.y = lin_to_acescct(crgb.y);
    crgb.z = lin_to_acescct(crgb.z);
  } else if (working_colorspace == 2) {
    crgb.x = lin_to_acescc(crgb.x);
    crgb.y = lin_to_acescc(crgb.y);
    crgb.z = lin_to_acescc(crgb.z);
  }

  crgb = mix(rgb, crgb, select);

  gl_FragColor = vec4(crgb, alpha);
}
