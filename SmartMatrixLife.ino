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

#include <SmartMatrix_32x32.h>
#include <FastLED.h>

#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds[NUM_LEDS];

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

Cell world[MATRIX_WIDTH][MATRIX_HEIGHT];
long density = 50;
int generation = 0;

void setup() {
  LEDS.addLeds<SMART_MATRIX>(leds, NUM_LEDS);
  LEDS.setBrightness(32);
}

void loop() {
  if (generation == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    randomFillWorld();

    if(alternateBlur)
        blur = !blur;

    if(switchPalette)
        chooseNewPalette();

    if(randomBlur)
        blurAmount = ((float)random(50, 90)) / 100.0;
  }

  // Display current generation
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    for (int j = 0; j < MATRIX_HEIGHT; j++) {
      if(blur) {
        leds[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
      }
      else if (world[i][j].alive == 1) {
        leds[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
      }
      else {
        leds[XY(i, j)] = CRGB::Black;
      }
    }
  }

  // Birth and death cycle
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
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
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      world[x][y].prev = world[x][y].alive;
    }
  }

  generation++;
  if (generation >= 256)
    generation = 0;

  LEDS.show();
  LEDS.delay(33);
}

void randomFillWorld() {
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    for (int j = 0; j < MATRIX_HEIGHT; j++) {
      if (random(100) < density) {
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
  return (world[(x + 1) % MATRIX_WIDTH][y].prev) +
    (world[x][(y + 1) % MATRIX_HEIGHT].prev) +
    (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][y].prev) +
    (world[x][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev) +
    (world[(x + 1) % MATRIX_WIDTH][(y + 1) % MATRIX_HEIGHT].prev) +
    (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][(y + 1) % MATRIX_HEIGHT].prev) +
    (world[(x + MATRIX_WIDTH - 1) % MATRIX_WIDTH][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev) +
    (world[(x + 1) % MATRIX_WIDTH][(y + MATRIX_HEIGHT - 1) % MATRIX_HEIGHT].prev);
}

uint16_t XY( uint8_t x, uint8_t y) {
  return (y * MATRIX_WIDTH) + x;
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
