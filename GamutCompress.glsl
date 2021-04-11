uniform sampler2D frontTex, matteTex, selectiveTex;
uniform float cyan, magenta, yellow, adsk_result_w, adsk_result_h;
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

void main() {
  vec2 coords = gl_FragCoord.xy / vec2( adsk_result_w, adsk_result_h );
  // source pixels
  vec3 rgb = texture2D(frontTex, coords).rgb;
  float alpha = texture2D(matteTex, coords).g;
  float select = texture2D(selectiveTex, coords).g;

  // Linearize working colorspace
  if (working_colorspace == 1) {
    rgb.x = acescct_to_lin(rgb.x);
    rgb.y = acescct_to_lin(rgb.y);
    rgb.z = acescct_to_lin(rgb.z);
  } else if (working_colorspace == 2) {
    rgb.x = acescc_to_lin(rgb.x);
    rgb.y = acescc_to_lin(rgb.y);
    rgb.z = acescc_to_lin(rgb.z);
  } 

  // Amount of outer gamut to affect
  vec3 th = 1.0-threshold;
  
  // Distance limit: How far beyond the gamut boundary to compress
  vec3 dl = 1.0+vec3(cyan, magenta, yellow);

  // Calculate scale so compression function passes through distance limit: (x=dl, y=1)
  vec3 s;
  s.x = (1.0-th.x)/sqrt(max(1.001, dl.x)-1.0);
  s.y = (1.0-th.y)/sqrt(max(1.001, dl.y)-1.0);
  s.z = (1.0-th.z)/sqrt(max(1.001, dl.z)-1.0);

  // Achromatic axis
  float ac = max(rgb.x, max(rgb.y, rgb.z));

  // Inverse RGB Ratios: distance from achromatic axis
  vec3 d = ac==0.0?vec3(0.0):(ac-rgb)/abs(ac);

  vec3 cd; // Compressed distance
  // Parabolic compression function: https://www.desmos.com/calculator/nvhp63hmtj
  if (!invert) {
    cd.x = d.x<th.x?d.x:s.x*sqrt(d.x-th.x+s.x*s.x/4.0)-s.x*sqrt(s.x*s.x/4.0)+th.x;
    cd.y = d.y<th.y?d.y:s.y*sqrt(d.y-th.y+s.y*s.y/4.0)-s.y*sqrt(s.y*s.y/4.0)+th.y;
    cd.z = d.z<th.z?d.z:s.z*sqrt(d.z-th.z+s.z*s.z/4.0)-s.z*sqrt(s.z*s.z/4.0)+th.z;
  } else {
    cd.x = d.x<th.x?d.x:pow(d.x-th.x+s.x*sqrt(s.x*s.x/4.0),2.0)/(s.x*s.x)-s.x*s.x/4.0+th.x;
    cd.y = d.y<th.y?d.y:pow(d.y-th.y+s.y*sqrt(s.y*s.y/4.0),2.0)/(s.y*s.y)-s.y*s.y/4.0+th.y;
    cd.z = d.z<th.z?d.z:pow(d.z-th.z+s.z*sqrt(s.z*s.z/4.0),2.0)/(s.z*s.z)-s.z*s.z/4.0+th.z;
  }

  // Inverse RGB Ratios to RGB
  vec3 crgb = ac-cd*abs(ac);

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