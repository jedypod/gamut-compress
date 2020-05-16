# Gamut Compression
This is a set of tools to compress out of gamut values back into gamut.

### Info
Out of gamut colors can ocurr with modern digital cinema cameras.  Using the original camera gamut, there would be no problem, but often when doing visual effects or grading, we need to work in a different colorspace like ACEScg. Since ACEScg is designed to operate within the spectral locus, this is where out of gamut colors can appear.

Cameras can generate color outside of the spectral locus because the spectral response curves of the camera do not match the spectral response curves of the human eye, making the camera a non-colorimetric device which does not satisfy the Luther-Ives criterion. 

Out of gamut colors often ocurr with highly saturated light sources like police lights, neon lights, or lasers. Re-mapping these colors back into gamut is necessary for pleasing color reproduction, and for working.

### Usage
Method specifies the type of compression curve to use. Tanh is a hyperbolic tangent function which has a very  smooth rolloff. This method tends to preserve the appearance of colors very well. Exp is a natural exponential compression function which is  a bit more agressive slope. Simple has a more aggressive slope, and tends to change the appearance of colors a bit more, but can be good when used with creative intent. Good base is 0.8 for limits when  using this compression method.

Threshold is the percentage of the core gamut to affect. A value of 0 would be a hard clip, a value of 0.2 would affect  the outer 20% of the gamut's most  saturated colors.


The cyan / magenta / yellow limits allow you to adjust the amount of compression per color component.  For example increasing the magenta limit will push blues more cyan. A value of 0 is no compression. A value of 1 compresses to the gamut boundary. And values above 1 up to a max of 1/(1-threshold) will compress more than the  gamut boundary. Note a value at max will be a hard clip at the  threshold (inside the gamut boundary!) and is probably not something you want!

Inverting the gamut compression is possible but should be used with an excess of caution, because saturated values can be pushed well outside of the spectral locus. 


### About
Written by Jed Smith with [tons of help](https://community.acescentral.com/t/rgb-saturation-gamut-mapping-approach-and-a-comp-vfx-perspective) from the [ACES Gamut Mapping Virtual Working Group](https://community.acescentral.com/c/aces-development-acesnext/vwg-aces-gamut-mapping-working-group)
