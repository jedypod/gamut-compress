# GamutCompress
is a tool to compress out of gamut rgb color values back into gamut.

## Context
Out of gamut colors can ocurr with modern digital cinema cameras. If using the camera manufacturer's color pipeline there would be no issue. However with color management pipelines like the [Academy Color Encoding System](https://www.oscars.org/science-technology/sci-tech-projects/aces), there is a working colorspace into which all source imagery is transformed using an IDT or Input Device Transform. 

In the case of ACES this working colorspace is ACEScg. The ACEScg gamut is smaller than most digital cinema camera gamuts. Because of this, when the IDT is applied, highly saturated colors can go out of gamut. Usually when this happens, one of the RGB components is a high value, and one or more of the other components are negative values.

If cameras were colorimetric devices that satisfied the [Luther condition](https://en.wikipedia.org/wiki/Tristimulus_colorimeter), there would be no problem. But, because this is not the case, most cameras will hapilly generate RGB tristimulus values that lie outside of the spectral locus. This is possible because the spectral locus represents the bounds of color perceivable by the spectral response curves of the human eye. A digital cinema camera's sensor has spectral response curves that are not the same!

These out of gamut colors most often ocurr with highly saturated light sources like police lights, neon lights, or lasers.

This tool was developed as part of the [ACES Gamut Mapping Virtual Working Group](https://community.acescentral.com/c/aces-development-acesnext/vwg-aces-gamut-mapping-working-group/80). For more info check out the [working group proposal](https://www.dropbox.com/s/5hz8e07ydx0d2bm/ACES_Gamut_Mapping_Working_Group_Proposal_Approved.pdf).

Here is a contactsheet of [sample images](https://www.dropbox.com/sh/u6z2a0jboo4vno8/AAB-10qcflhpr0C5LWhs7Kq4a?dl=0) showing the visual appearance of out of gamut colors.

[<img src=https://github.com/jedypod/gamut-compress/blob/master/images/collage.rrt.jpg width=48% height=48%/>](https://github.com/jedypod/gamut-compress/blob/master/images/collage.rrt.jpg?raw=true) [<img src=https://github.com/jedypod/gamut-compress/blob/master/images/collage_compressed.rrt.jpg width=48% height=48%/>](https://github.com/jedypod/gamut-compress/blob/master/images/collage_compressed.rrt.jpg?raw=true)

On the left are the original images with the ACES Rec.709 RRT display rendering transform applied. On the right are the same images, but with gamut compression applied.

If the display rendering transform does not elegantly handle out of gamut colors, artifacts such as those seen above can occur. The negative values in scene-linear of these out of gamut colors can pose issues for image processing as well.

Compressing these out of gamut colors back into gamut is useful for pleasing color reproduction, and for reducing issues in VFX compositing and DI.

## Implementations
DCC Implementations for the following software packages are included. For specific installation instructions, check out the documentation links for each package below.
- [Blinkscript for Nuke](docs/doc-nuke.md) (A [non-blinkscript node](GamutCompress.nk) is also provided for Nuke Non-Commercial).
- [Fuse for Fusion or Resolve Lite](docs/doc-fusion.md)
- [DCTL for Resolve Studio](docs/doc-resolve.md)
- [Matchbox for Flame, Scratch and Baselight](docs/doc-flame.md)

## Download
You can grab the latest "stable" release from the [Releases page](https://github.com/jedypod/gamut-compress/releases), or you can clone the repo to get the latest work. 

`git clone git@github.com:jedypod/gamut-compress.git`

## Documentation
For more information about [how the it works](/docs/gamut-compress-algorithm.md) and [how to use it](/docs/gamut-compress-documentation.md), please check out the [documentation](/docs).

## About
Written by Jed Smith with [tons of help](https://community.acescentral.com/t/rgb-saturation-gamut-mapping-approach-and-a-comp-vfx-perspective) from the [ACES Gamut Mapping Virtual Working Group](https://community.acescentral.com/c/aces-development-acesnext/vwg-aces-gamut-mapping-working-group). 

Also huge thanks to the contributions of [@nick-shaw](https://github.com/nick-shaw) for the initial versions of the DCTL and Matchbox implementations, and to Jacob Danell for the first version of the Fuse implementation.