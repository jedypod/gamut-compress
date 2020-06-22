# GamutCompress Usage & Workflow

This is written assuming you have read and understand the [how it works document](/docs/gamut-compress-algorithm.md).


# Goals Usages Needs
The needs of a tool depends on the context in which it is used. I see 3 main contexts in which gamut compression could be used: ( there may be more)

**VFX Vendor**
+ Goal
    * Remove problematic negative RGB color components to reduce issues with image manipulation.
+ Usage
    * Gamut compression applied to plates on ingest, reversed on delivery.
+ Needs  
    * Exact inverse transform.
    * Plausible color rendering to enable good decision making in image manipulation.
    * Fine control and ability to optimize and customize for a specific circumstance. Per source camera, per sequence, or even per shot if needed.
    * Tools for calculating and visualizing distances of source images or source gamuts, to aid in customization and optimization.  

**DI Suite**
+ Goal
    * To creatively manipulate out of gamut colors into a pleasing result.
+ Usage
    * Applied in a creative look-driven context for final image output.
+ Needs
    * Creative flexibility.
    * Intuitive parameters.
    * Plausible, good looking result.
    * Exact inverse transform not needed in most circumstances since this transform would be applied prior to final output. However it might might be necessary for collaboration with a VFX facility.

**Display Rendering Transform**
+ Goal  
To aide in the rendering of out of gamut colors on a display.
+ Usage  
Applied in scene-referred space before the display rendering transform.  
(This algorithm only works in scene-referred, and is not suitable for more complex display-referred gamut mapping).
+ Needs
    * Handle any and all possible source imagery.
    * Plausible good looking result.
    * No exposed or adjustable parameters.

With that outline of possible goals and needs, we will now talk about how to use the tool, and make some workflow reccomendations.



# Parameters
![GamutCompress Nuke UI](/images/screenshots/GamutCompress_nuke-ui.png)

Here is a description of what each of the GamutCompress parameters do.


## Threshold
Treshold controls the percentage of the outer gamut to affect. A value of 0.2 will compress out of gamut values into the outer 20% of the gamut. The inner 80% of the gamut core will not be affected. In the Nuke nodes you are able to adjust these per color component. For example if you wanted to protect reds and greens a little more than blue, you could set the threshold a bit higher for blue.


## Power
Control the aggressiveness of the compression curve. Higher power values result in more compressed values distributed closer to the gamut boundary. This will increase the "color purity" or "saturation" once the image has been rendered through a display transform.  

A power value of 1 is equal to a Reinhard compression curve. Higher values have a smoother transition and approach C2 continuity.  

Be careful of higher power values if an accurate inverse transform is important to you. Higher power values cause the compression curve to go more flat. Depending on the precision of your processing space, a flatter curve is more likely to result in quantization errors. This will result in differences between the inverse transform and the original.


## Shadow Rolloff
The shadow rolloff parameter smoothly reduces the effect of the gamut compression below a specified threshold achromatic value. 

We must divide the inverse RGB ratios by the achromatic value to get a distance value of 1 at the gamut boundary. When the achromatic value gets very small, this can have a very big effect on the resulting distance. The bigger the distance value, the more likely it is to have quantization errors in the inverse direction.

The closer the luminance of an RGB value gets to 0, the less our eyes perceive it as color. 

The purpose of this parameter is to reduce invertibility issues in shadow grain. Avoiding modifications to shadow grain may also be desireable in a VFX pipeline to avoid causing issues with shadow matching in composites, when the inverse gamut compression transform is applied. 

When this parameter is used, the algorithm is no longer exposure invariant. That is, when applying the gamut compression on an image, and on the same image exposed up by 6 stops, there will be a small difference in shadow grain. If exposure invariance is important to you feel free to disable this by setting the shadow rolloff to 0. 


## Max Distance Limits
Max Distance is the distance outside of the gamut boundary to compress to the gamut boundary. For example, a value of 0.2 will compress distance values of 1.2 to a value of 1.0.

Individual controls are given for each color component. They are named cyan magenta and yellow because they actually control the max distance from the edge of the triangle, not the corner. For example, boosting the value of the cyan distance limit will increase the distance from which values are mapped to the edge between the blue and green corners of the triangle (to use simple 2 dimensional terminology).

The distance limits also control the RGB ratios of the compressed values, which affects the apparent hue, once the image has been rendered. It may be desireable to get a close match to the original camera color-rendering. In order to do this, a good starting point is to set the max distance parameters according to the original source gamut. There is a utility called [CalculateDistance](/utilities/CalculateDistance.nk) which can aide in this calculation. It can calculate the maximum possible distance for each color component given a processing gamut and a source gamut. Or you can use an input image source to calculate from. 

You should try for a good set of default values that handles as much variety of source imagery as possible. Exceptions may be necessary, but this should be the starting point. 


## Inverse Direction
As hinted at above, exact inversion of the gamut compression is possible. 

Using inverse gamut compression should be avoided at all costs if possible. There is a high likelihood of breakage and things going horrible wrong. 

With that disclaimer out of the way, inverse gamut compression may be a necessity in some circumstances such as in VFX pipelines.

If the working gamut of the VFX pipeline is different than the gamut of the original footage from the client, normally a transform would be applied on plate ingest to convert the source gamut to the working gamut. Work would ocurr in the working gamut. CG would be rendered. Compositing would be done. 

On delivery, there would be an inverse transform to convert working gamut back to source gamut. This would result in an exact match between the footage from the client and the comp delivered from the VFX vendor, except where work has been done. 

Forward and inverse gamut compression has good results when performed on the same image. If new highly saturated color values are introduced to the gamut compressed image, the results may become extreme and unpredictable when  inverse gamut compression is applied. 

Therefore it is very important to be warey of this and use an excess of caution. Steps should be taken to carefully monitor any colors that are created in the region between the threshold distance and the gamut boundary. 

There is a tool called [VisualizeDistance](/utilities/VisualizeDistance.nk) which may help with this. It calculates the distance of the input image, and shows values visually that are beyond a specified threshold. 