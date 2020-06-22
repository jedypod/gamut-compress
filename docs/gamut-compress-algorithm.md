# The Gamut Compression Algorithm

## How it works
The gamut compression algorithm is actually very simple. 

It works in a scene-linear floating-point domain. Because the scene-linear data has not been transformed into a display-referred colorspace yet, perceptual attributes like luminance, hue, and saturation don't yet have meaning. Therefore the algorithm works purely with the RGB color components. It is similar in nature to a despill algorithm.

The gamut compression algorithm runs per-pixel, and is controlled with a small set of parameters. These parameters control how much of the core "confidence gamut" to protect, how much to compress each of the color components, and the shape of the compression curve.

## Representations
To achieve this gamut compression, we need to construct a few different representations of the input image.
- **Achromatic**  
Similar to luminance, this represents the achromatic or neutral axis of the color cube. Achromatic is constructed by taking the maximum of the R, G, and B color components.  
`achromatic = max(r,g,b)`  
Constructing the neutral axis this way allows us to leave one of the color components unaffected through the gamut mapping transformation, which makes inversion possible.  
- **Distance**  
Similar in concept to saturation, distance represents the distance of a color component from the achromatic axis.  
`distance = (achromatic - rgb) / achromatic`.  
One could think about this as the euclidean distance, but in one dimension this is simply the absolute value. And this can be represented even more simply by subtracting the color component from the achromatic value. Since the result will never be negative, the absolute value calculation is not necessary.   
This calculation gives us the distance, or the "inverse rgb ratios". However we still have a problem. We don't know where the gamut boundary is. For our distance value to be useful, we need it to represent in concrete terms what is in gamut and what is out of gamut.   
Fortunately this is a simple fix. All we need to do is normalize the distance values by dividing by achromatic. This gives us a distance which is 1.0 at the gamut boundary.

## Pictures Tell 1,000 Words
This is all very abstract, so let's explain these concepts with some pictures. 

![Colorwheel CIE 1931 xy - ACEScg](/images/screenshots/colorwheel_acescg_1931_xy.png)

Here is a CIE 1931 xy chromaticity diagram. I've taken a colorwheel and increased the saturation equally in all directions. I've plotted this colorhweel assuming the input RGB values are ACEScg. I've plotted the triangle of the ACEScg gamut as well.

RGB values within the ACEScg triangle are positive. RGB values outside of the triangle have negative values in one or two of their components. If you look at the pixel value being sampled just outside of the blue primary, the R and G components are negative while the B component is positive. These negative components are the problematic ones we are trying to fix by mapping them back inside the bounds of the gamut.

Note that the xy chromaticity diagram is a simplified 2-dimensional representation of this inherently 3-dimensional data. However it serves to illustrate the concept in a simple way.

![Colorwheel Distance](/images/screenshots/colorwheel_acescg_distance_1931_xy.png)

Here is the same image but transformed into the the distance representation described above. Values inside of the ACEScg triangle are between 0 and 1, with zero being the CIE xy location of the whitepoint of the image (D60, or `0.32168 0.33767` in xy chromaticity coordinates). This is the location of the achromatic axis.

So inside the triangle we are all good. But outside the triangle is where the problematic values are. These values have a distance that is greater than 1.

![Colorwheel Distance Gamma Down](/images/screenshots/colorwheel_acescg_distance_gamma_down_1931_xy.png)

Here you can see the same distance image, but with a strong gamma down. You can see the values near the gamut boundary are near 1.

## Compression
So we need to take distance values above 1, and compress them down to 1. To do this we can use a compression function. A function which is linear (y=x) below a specified threshold. Above that threshold y values are mapped lower in a smooth curve.

In order to effectively compress values though, we need space to compress them into. If we compress values above 1 to 1, that is the same thing as clipping those values. We want a smooth rolloff of our curve that evenly distributes the compressed values into the space between a threshold value and the gamut boundary. The threshold is where the curve starts compressing, and values below that threshold won't be affected.

Out of gamut values don't tend to have huge distance values. The size of the distance values varies depending on the original camera gamut. For example Alexa Wide Gamut can have larger distance values near the blue primary, but usually no out of gamut values around the red primary. This is because of the shape of the camera gamut compared to ACEScg. RedWideGamutRGB is more likely to have out of gamut values around the red primary because of it's distance compared to the red primary of ACEScg.

If we carelessly compress infinite distance to the gamut boundary, this makes very innefficient usage of the compression space between the threshold and the gamut boundary. We might be wasting 99% of it!

Therefore it is very important to carefully choose how far beyond the gamut boundary we are compressing _to_ the gamut boundary.

We control this with the Max Distance Limit parameters. These are split based on the color components so that we have precise control to optimize for the source camera gamut. 

In addition to efficient usage of the compression space, the distance limits affect the output RGB ratios for the compressed out of gamut colors. Once the image is run through a display rendering transform, these ratios have a big impact on the hue. Ideally we would be compressing out of gamut values into plausible colors. When the distance limits are set according to the source camera gamut, the hue of those compressed colors is more similar to the color rendering pipeline of the original camera data.

We also want control control over the agressiveness of the compression curve. Aggressiveness can be thought of here as how quickly the curve transitions from linear to compressed. A less aggressive curve that transitions more quickly will result in compressed values being mapped closer to the threshold, and therefore the colors will appear less saturated. 

![Less Aggressive Compression Function Curve](/images/screenshots/compression_function.png)

Here's an example of a less aggressive compression curve.

![Aggressive Function Curve](/images/screenshots/compression_function_aggressive.png)

And here's an example of a more aggressive curve.

A more aggressive curve will push compressed values further out towards the gamut boundary. These colors will stay very saturated once rendered through a display transform.

To control the aggressiveness of the curve we can use the power parameter. Higher power settings have a more aggressive curve and result in the compressed color values being closer to the gamut boundary and more saturated.

There is [an interactive plot](https://www.desmos.com/calculator/54aytu7hek) of the compression curve that is being used, if you would like to play around with the parameters and get a feel for what effect they have on the shape of the curve.

## Getting Back to Reality
Finally once we have our compressed distance, we can reconstruct our RGB image from our achromatic and newly compressed distance representations. The math for this is just as simple as getting here:

`rgb = achromatic - compressed_distance * achromatic`


If you've read this far and don't feel too overwhelmed, feel free to read through [how to use the tool](/docs/gamut-compress-documentation.md).