uniform sampler2D frontTex, matteTex, selectiveTex;
uniform float power, shd_rolloff, cyan, magenta, yellow, adsk_result_w, adsk_result_h;
uniform int working_colorspace;
uniform bool invert;
uniform vec3 threshold;

// calc hyperbolic tangent
float tanh( float val) {
  float f = exp(2.0*val);
  return (f-1.0)/(f+1.0);
}

// convert acescg to acescct
float lin_to_acescct(float val) {
  if (val <= 0.0078125) {
    return 10.5402377416545 * val + 0.0729055341958355;
  } else {
    return (log2(val) + 9.72) / 17.52;
  }
}

// convert acescct to acescg
float acescct_to_lin(float val) {
  if (val > 0.155251141552511) {
    return pow( 2.0, val*17.52 - 9.72);
  } else {
    return (val - 0.0729055341958355) / 10.5402377416545;
  }
}

// convert acescg to acescc
float lin_to_acescc(float val) {
  if (val <= 0.0) {
    return -0.3584474886; // =(log2( pow(2.,-16.))+9.72)/17.52
  } else if (val < pow(2.0, -15.0)) {
    return (log2(pow(2.0, -16.0)+val*0.5)+9.72)/17.52;
  } else { // (val >= pow(2.,-15))
    return (log2(val)+9.72)/17.52;
  }
}

// convert acescc to acescg
float acescc_to_lin(float val) {
  if (val < -0.3013698630) { // (9.72-15)/17.52
    return (pow(2.0, val*17.52-9.72) - pow(2.0, -16.0))*2.0;
  } else if (val < (log2(65504.0)+9.72)/17.52) {
    return pow(2.0, val*17.52-9.72);
  } else { // (val >= (log2(HALF_MAX)+9.72)/17.52)
    return 65504.0;
  }
}

// calculate compressed distance
float compress(float x, float l, float t, float p, bool invert) {
  float cdist;
  // power(p) compression function plot https://www.desmos.com/calculator/54aytu7hek
  float s = (l-t)/pow(pow((1.0-t)/(l-t),-p)-1.0,1.0/p); // calc y=1 intersect
  if (l < 1.0001) {
    return x; // disable compression, avoid nan
  }
  if (x < t) {
    cdist = x;
  } 
  else {
    if (!invert) {
      cdist = t+s*((x-t)/s)/(pow(1.0+pow((x-t)/s,p),1.0/p)); // compress
    } else {
      if (x > (t + s)) {
        cdist = x; // avoid singularity
      }
      cdist = t+s*pow(-(pow((x-t)/s,p)/(pow((x-t)/s,p)-1.0)),1.0/p); // uncompress
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

  // Working colorspace to linear
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
  
  // lim is the distance beyond the gamut boundary that will be compressed to the gamut boundary.
  // lim = 0.2 will compress from a distance of 1.2 from achromatic to 1.0 (the gamut boundary).  
  vec3 lim;
  lim = vec3(cyan+1.0, magenta+1.0, yellow+1.0);

  // achromatic axis 
  float ach = max(rgb.x, max(rgb.y, rgb.z));

  // achromatic shadow rolloff
  float ach_shd;
  if (shd_rolloff < 0.004) {
    // disable shadow rolloff functionality. 
    // values below 0.004 cause strange behavior, actually increasing distance in some cases.
    // if ach < 0.0 and shd_rolloff is disabled, take absolute value. This preserves negative components after compression.
    ach_shd = abs(ach);
  } else {
    // lift ach below threshold using a tanh compression function. 
    // this reduces large distance values in shadow grain, which can cause differences when inverting.
    ach_shd = 1.0-((1.0-ach)<(1.0-shd_rolloff)?(1.0-ach):(1.0-shd_rolloff)+shd_rolloff*tanh((((1.0-ach)-(1.0-shd_rolloff))/shd_rolloff)));
  } 

  // distance from the achromatic axis for each color component aka inverse rgb ratios
  // distance is normalized by achromatic, so that 1.0 is at gamut boundary. avoid 0 div
  vec3 dist;
  dist.x = ach_shd == 0.0 ? 0.0 : (ach-rgb.x)/ach_shd;
  dist.y = ach_shd == 0.0 ? 0.0 : (ach-rgb.y)/ach_shd;
  dist.z = ach_shd == 0.0 ? 0.0 : (ach-rgb.z)/ach_shd;

  // compress distance with user controlled parameterized shaper function
  vec3 cdist = vec3(
    compress(dist.x, lim.x, thr.x, power, invert),
    compress(dist.y, lim.y, thr.y, power, invert),
    compress(dist.z, lim.z, thr.z, power, invert));

  // recalculate rgb from compressed distance and achromatic
  // effectively this scales each color component relative to achromatic axis by the compressed distance
  vec3 crgb = vec3(
    ach-cdist.x*ach_shd,
    ach-cdist.y*ach_shd,
    ach-cdist.z*ach_shd);

  // Linear to working colorspace
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