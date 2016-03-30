/*
 * Aurora: https://github.com/pup05/SmartMatrixLife
 * Copyright (c) 2014 Jason Coon
 *
 * Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
 * which, in turn, was "Adapted from the Life example on the Processing.org site"
 *
 * Made much more colorful by J.B. Langston: https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <SmartMatrix3.h>
#include <FastLED.h>

#define COLOR_DEPTH 24                  // This sketch and FastLED uses type `rgb24` directly, COLOR_DEPTH must be 24
const uint8_t kMatrixWidth = 32;        // known working: 32, 64, 96, 128
const uint8_t kMatrixHeight = 32;       // known working: 16, 32, 48, 64
const uint8_t kRefreshDepth = 36;       // known working: 24, 36, 48
const uint8_t kDmaBufferRows = 4;       // known working: 2-4, use 2 to save memory, more to keep from dropping frames and automatically lowering refresh rate
const uint8_t kPanelType = SMARTMATRIX_HUB75_32ROW_MOD16SCAN;   // use SMARTMATRIX_HUB75_16ROW_MOD8SCAN for common 16x32 panels
const uint8_t kMatrixOptions = (SMARTMATRIX_OPTIONS_NONE);      // see http://docs.pixelmatix.com/SmartMatrix for options
const uint8_t kBackgroundLayerOptions = (SM_BACKGROUND_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);


#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

// blur between each frame of the simulation
boolean blur = true;

// adjust the amount of blur
float blurAmount = 0.5;

boolean randomBlur = true;

// alternate between blurred and not blurred each time a new world is filled
boolean alternateBlur = true;

// switch to a random palette each time a new world is filled
boolean switchPalette = true;

CRGBPalette16 currentPalette = PartyColors_p;

class Cell {
public:
  byte alive =  1;
  byte prev =  1;
  byte hue = 6;  
  byte brightness;
};

Cell world[kMatrixWidth][kMatrixHeight];
long density = 50;
int generation = 0;

void randomFillWorld() {
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      if (random(100) < (unsigned long)density) {
        world[i][j].alive = 1;
        world[i][j].brightness = 255;
      }
      else {
        world[i][j].alive = 0;
        world[i][j].brightness = 0;
      }
      world[i][j].prev = world[i][j].alive;
      world[i][j].hue = 0;
    }
  }
}

int neighbours(int x, int y) {
  return (world[(x + 1) % kMatrixWidth][y].prev) +
    (world[x][(y + 1) % kMatrixHeight].prev) +
    (world[(x + kMatrixWidth - 1) % kMatrixWidth][y].prev) +
    (world[x][(y + kMatrixHeight - 1) % kMatrixHeight].prev) +
    (world[(x + 1) % kMatrixWidth][(y + 1) % kMatrixHeight].prev) +
    (world[(x + kMatrixWidth - 1) % kMatrixWidth][(y + 1) % kMatrixHeight].prev) +
    (world[(x + kMatrixWidth - 1) % kMatrixWidth][(y + kMatrixHeight - 1) % kMatrixHeight].prev) +
    (world[(x + 1) % kMatrixWidth][(y + kMatrixHeight - 1) % kMatrixHeight].prev);
}

uint16_t XY( uint8_t x, uint8_t y) {
  return (y * kMatrixWidth) + x;
}

void chooseNewPalette() {
    switch(random(0, 8)) {
        case 0:
            currentPalette = CloudColors_p;
            break;

        case 1:
            currentPalette = ForestColors_p;
            break;

        case 2:
            currentPalette = HeatColors_p;
            break;

        case 3:
            currentPalette = LavaColors_p;
            break;

        case 4:
            currentPalette = OceanColors_p;
            break;

        case 5:
            currentPalette = PartyColors_p;
            break;

        case 6:
            currentPalette = RainbowColors_p;
            break;

        case 7:
        default:
            currentPalette = RainbowStripeColors_p;
            break;
    }
}

void setup() {
  matrix.addLayer(&backgroundLayer); 
  matrix.begin();
  matrix.setBrightness(32);
}

void loop() {
  rgb24 *buffer = backgroundLayer.backBuffer();

  if (generation == 0) {
    fill_solid((CRGB*)buffer, NUM_LEDS, CRGB::Black);

    randomFillWorld();

    if(alternateBlur)
        blur = !blur;

    if(switchPalette)
        chooseNewPalette();

    if(randomBlur)
        blurAmount = ((float)random(50, 90)) / 100.0;
  }

  // Display current generation
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      if(blur) {
        buffer[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
      }
      else if (world[i][j].alive == 1) {
        buffer[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
      }
      else {
        buffer[XY(i, j)] = (CRGB)CRGB::Black;
      }
    }
  }

  // Birth and death cycle
  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      // Default is for cell to stay the same
      if (world[x][y].brightness > 0 && world[x][y].prev == 0)
        world[x][y].brightness *= blurAmount;
      int count = neighbours(x, y);
      if (count == 3 && world[x][y].prev == 0) {
        // A new cell is born
        world[x][y].alive = 1;
        world[x][y].hue += 2;
        world[x][y].brightness = 255;
      } 
      else if ((count < 2 || count > 3) && world[x][y].prev == 1) {
        // Cell dies
        world[x][y].alive = 0;
      }
    }
  }

  // Copy next generation into place
  for (int x = 0; x < kMatrixWidth; x++) {
    for (int y = 0; y < kMatrixHeight; y++) {
      world[x][y].prev = world[x][y].alive;
    }
  }

  generation++;
  if (generation >= 256)
    generation = 0;

  backgroundLayer.swapBuffers();
  LEDS.delay(33);
}
