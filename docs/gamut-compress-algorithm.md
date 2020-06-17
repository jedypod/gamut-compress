# The Gamut Compression Algorithm
![Colorwheel CIE 1931 xy - ACEScg](/images/screenshots/colorwheel_acescg_1931_xy.png)
To help visualize this description we will use this image: a colorwheel plotted in CIE 1931 2 degree standard observer xy chromaticity diagram.

The gamut compression algorithm is actually very simple. Because it works in a scene-referred linear domain, perceptual attributes like luminance, hue, and saturation don't have a meaning. Therefore it works in a very simple way purely with the RGB color components. 

The algorithm is built to run per-pixel given a basic set of user-defined parameters. It constructs two representations of the image:
- Achromatic: Similar to luminance, this represents the achromatic axis in the color cube. Achromatic is constructed by taking the maximum of the R, G, and B color components. `achromatic = max(r,g,b)`
- Distance: Similar in concept to saturation, distance represents the distance of an RGB triplet from the achromatic axis. `distance = (achromatic - rgb) / achromatic`. 

![Colorwheel Distance](/images/screenshots/colorwheel_acescg_distance_1931_xy.png)

Here is what the above colorwheel image looks like transformed into the distance representation.

Dividing by achromatic makes it so that the gamut boundary is a value of 1.0, and anything beyond it is above 1.0.

![Colorwheel Distance Gamma Down](images/screenshots/colorwheel_acescg_distance_gamma_down_1931_xy.png)

This allows us to compress the distance using a tonemapping compression function, and then transform achromatic and distance back into an RGB image.
- `rgb = achromatic - distance * achromatic` 


### Usage
![GamutCompress Nuke UI](/images/screenshots/GamutCompress_nuke-ui.png)

Threshold is the percentage of the outer gamut to affect. A value of 0.2 will compress out of gamut values into the outer 20% of the gamut.

Max Distance is the distance past the gamut boundary to map to the gamut boundary. For example, a value of 0.2 will compress distance values of 1.2 to a value of 1.0. Individual controls are given for each color component (`cyan, magenta, and yellow`). These controls can be used to tune the look of the compression creatively, or to optimize for different source gamuts. Alexa Wide Gamut and RedWideGamutRGB have very different maximum possible distances for each of the color components, because their primaries are very different.

Method specifies the type of compression curve to use. Currently the master branch only has a simple Reinhard compression curve enabled, but if you check out the dev branch there are a few different experimental options. 

Inverting the gamut compression exactly is also possible. This workflow might be necessary in visual effects pipelines where the source plates need to be delivered back to the client exactly as they came in. There should be an excess of caution used with this workflow however. If highly saturated colors in the outer 20% of the color gamut are uncompressed, the results might be very extreme and unpredictable. Efforts should be undertaken to watch this eventuality very closely if this workflow is used.

![Resolve UI](/images/screenshots/GamutCompress_resolve-ui.png)

A DCTL for Blackmagic Resolve is also provided. This is based on the [initial version written by Nick Shaw](https://github.com/nick-shaw/gamut_mapping). Huge thanks to him for putting that together! He also has a Matchbox shader in his repo which might be of use in Baselight or Flame.
