![Resolve UI](/images/screenshots/GamutCompress_resolve-ui.png)

A DCTL for Blackmagic Resolve is also provided. This is based on the [initial version written by Nick Shaw](https://github.com/nick-shaw/gamut_mapping). Huge thanks to him for putting that together! He also has a Matchbox shader in his repo which might be of use in Baselight or Flame.



### Usage
![GamutCompress Nuke UI](/images/screenshots/GamutCompress_nuke-ui.png)

Threshold is the percentage of the outer gamut to affect. A value of 0.2 will compress out of gamut values into the outer 20% of the gamut.

Max Distance is the distance past the gamut boundary to map to the gamut boundary. For example, a value of 0.2 will compress distance values of 1.2 to a value of 1.0. Individual controls are given for each color component (`cyan, magenta, and yellow`). These controls can be used to tune the look of the compression creatively, or to optimize for different source gamuts. Alexa Wide Gamut and RedWideGamutRGB have very different maximum possible distances for each of the color components, because their primaries are very different.

Method specifies the type of compression curve to use. Currently the master branch only has a simple Reinhard compression curve enabled, but if you check out the dev branch there are a few different experimental options. 

Inverting the gamut compression exactly is also possible. This workflow might be necessary in visual effects pipelines where the source plates need to be delivered back to the client exactly as they came in. There should be an excess of caution used with this workflow however. If highly saturated colors in the outer 20% of the color gamut are uncompressed, the results might be very extreme and unpredictable. Efforts should be undertaken to watch this eventuality very closely if this workflow is used.

![Resolve UI](/images/screenshots/GamutCompress_resolve-ui.png)

A DCTL for Blackmagic Resolve is also provided. This is based on the [initial version written by Nick Shaw](https://github.com/nick-shaw/gamut_mapping). Huge thanks to him for putting that together! He also has a Matchbox shader in his repo which might be of use in Baselight or Flame.
