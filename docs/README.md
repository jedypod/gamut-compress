# Documentation
Here you can find documentation for the Gamut Compress tool. It's split up into a few documents so as to not be too overwhelming.

## [The Gamut Compression Algorithm](/docs/gamut-compress-algorithm.md)
## [Usage and Workflow](/docs/gamut-compress-documentation.md)

## DCC Implementation Documentation
Here is specific documentation for each of the DCC implemenetations. 

- [Nuke](/docs/doc-nuke.md)
- [Resolve](/docs/doc-resolve.md)
- [Fusion](/docs/doc-fusion.md)
- [Flame](/docs/doc-flame.md)



## Other
There are also a few nuke scripts preserved here which were used to present work to the Gamut Mapping VWG. 

[This is the Nuke script](/docs/aces_gamut_compression_vwg_presentation_20200521.nk) used in the presentation given for the ACES Gamut Mapping virtual working group on May 21st 2020.

Here are links to the [meeting notes](https://community.acescentral.com/t/notice-of-meeting-aces-gamut-mapping-vwg-meeting-13-5-21-2020/2891/2), and a [recording of the session](https://paper.dropbox.com/doc/Gamut-Mapping-Virtual-Working-Group--A0YcPBoTN8OjbKc3RCXgiKTtAg-1BByr3dSQvpjlYcOJlVgR).

This nuke script does contain some nodes like the [PlotChromaticity](https://github.com/jedypod/nuke-colortools/blob/master/toolsets/PlotChromaticity.nk) tool that rely on BlinkScript, which will not work in [Nuke Non-Commercial](https://www.foundry.com/products/nuke/non-commercial). However, the same functionality can be achieved using a [PlotPoints](https://github.com/jedypod/nuke-colortools/blob/master/toolsets/PlotPoints.nk) node, though with some performance penalties.

This nuke script contains exr images which are not in this git repository due to the size. You can [download them here](https://www.dropbox.com/sh/jbng5hfi6ofqlp8/AABLGGxd9PWYMxFAadHK5mspa?dl=0).  These images have been sourced from [acescentral](https://acescentral.com) and the [ACES Gamut Mapping VWG](https://www.dropbox.com/sh/u6z2a0jboo4vno8/AAB-10qcflhpr0C5LWhs7Kq4a?dl=0) dropbox. They have been resized to 2k and converted to ACES 2065-1 colorspace. These images are not a complete set but rather the ones I've found to be the most interesting.

If you download the image set to [/images/aces_gamut_mapping_vwg_images_ap0/](/images/aces_gamut_mapping_vwg_images_ap0/) the nuke script's relative paths should automatically find the images.

There is also the [script from the 2020-05-28 meeting here](/docs/aces_gamut_compression_vwg_presentation_20200528.nk).