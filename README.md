SmartMatrixLife
=================

Conway's Game of Life on a SmartMatrix 32x32 RGB LED display using the FastLED library.

[Demo Video](https://www.youtube.com/watch?v=lLjxPQNDcus):

[![Demo Video](http://img.youtube.com/vi/lLjxPQNDcus/0.jpg)](https://www.youtube.com/watch?v=lLjxPQNDcus)

SmartMatrix: http://pixelmatix.com

##### Controls (for all remotes):

* When not paused:
  * The arrow and select buttons will pause the simulation.
  * The "Randomly Fill World" button will fill the world with random cells.
* When paused: 
  * The arrow buttons will move the cursor.
  * The select button will toggle the highlighted cell.
  * The "Randomly Fill World" button will toggle between clearing and randomly filling the world.

##### Supported Infrared Remote Controls:

###### [SparkFun](https://www.sparkfun.com/products/11759)
[![SparkFun Remote Control](https://cdn.sparkfun.com/r/92-92/assets/parts/7/9/8/6/11759-01.jpg "SparkFun Remote Control")](https://www.sparkfun.com/products/11759)

Controls (left to right, top to bottom):

* Power
  * Press to turn on the display.
  * Press to cycle through brightness presets:
    * Off, 1%, 25%, 50%, 75%, 100%
* Pause/Play
* Next Palette
* Randomly Fill World
* Up
* Left
* Select
* Right
* Down

-------------------------------------------------------------------

###### [SmartMatrix](http://docs.pixelmatix.com/SmartMatrix/postkick.html)
[![SmartMatrix Remote Control](SmartMatrix-Remote.png "SmartMatrix Remote Control")](http://docs.pixelmatix.com/SmartMatrix/postkick.html)

Controls (left to right, top to bottom):

* Power
* Brightness Up
* Brightness Down
* Pause/Play
* Next Palette
* Randomly Fill World
* Up
* Left
* Select
* Right
* Back (unused)
* Down

--------------------------------------------------------

###### [Adafruit](https://www.adafruit.com/products/389)
[![Adafruit Remote Control](https://cdn-shop.adafruit.com/145x109/389-03.jpg "Adafruit Remote Control")](https://www.adafruit.com/products/389)

Controls (left to right, top to bottom):

* Brightness Down
* Power
* Brightness Up
* Unused
* Up
* Pause/Play
* Left
* Select
* Right
* Unused
* Down
* Unused
* Pause/Play
* Next Palette
* Randomly Fill World

###### Acknowledgements

Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
which, in turn, was "Adapted from the Life example on the Processing.org site"
 
Made much more colorful by J.B. Langston: https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b
