/*
   SmartMatrixLife: https://github.com/pup05/SmartMatrixLife
   Copyright (c) 2014 Jason Coon

   Portions of this code are adapted from Andrew: http://pastebin.com/f22bfe94d
   which, in turn, was "Adapted from the Life example on the Processing.org site"

   Made much more colorful by J.B. Langston: https://github.com/jblang/aurora/commit/6db5a884e3df5d686445c4f6b669f1668841929b

   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
   the Software, and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
   FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
   COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <SmartMatrix3.h>
#include <FastLED.h>
#include <IRremote.h>

#define IR_RECV_PIN 18

IRrecv irReceiver(IR_RECV_PIN);

#include "Commands.h"

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

rgb24 *buffer;

// blur between each frame of the simulation
boolean blur = false;

// adjust the amount of blur
float blurAmount = 0.5;

boolean isPaused = false;

boolean isCleared = false;

uint8_t cursorX = 0;
uint8_t cursorY = 0;

uint8_t brightnessIndex = 2;

const uint8_t brightnessCount = 5;
const uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };

uint8_t currentDelay = 33;

CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  OceanColors_p,
  CloudColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p,
  LavaColors_p,
};

uint8_t currentPaletteIndex = 0;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
const uint8_t paletteCount = ARRAY_SIZE(palettes);

CRGBPalette16 currentPalette = palettes[currentPaletteIndex];

class Cell {
  public:
    boolean alive = true;
    boolean prev = true;
    byte hue = 6;
    byte brightness;
};

Cell world[kMatrixWidth][kMatrixHeight];
long density = 50;
int generation = 0;

void randomFillWorld() {
  generation = 0;
  isCleared = false;

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      if (random(100) < (unsigned long)density) {
        world[i][j].alive = true;
        world[i][j].brightness = 255;
      }
      else {
        world[i][j].alive = false;
        world[i][j].brightness = 0;
      }
      world[i][j].prev = world[i][j].alive;
      world[i][j].hue = 0;
    }
  }
}

void clearWorld() {
  generation = 0;
  isCleared = true;

  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      world[i][j].alive = false;
      world[i][j].brightness = 0;
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

void powerOff() {
  fill_solid((CRGB*)buffer, NUM_LEDS, CRGB::Black);
  backgroundLayer.swapBuffers();
  buffer = backgroundLayer.backBuffer();

  while (true) {
    InputCommand command = readCommand(defaultHoldDelay);
    if (command != InputCommand::None)
      break;

    delay(250);
  }
}

void cyclePalette() {
  currentPaletteIndex = (currentPaletteIndex + 1) % paletteCount;
  currentPalette = palettes[currentPaletteIndex];
}

void cycleBrightness() {
  brightnessIndex = (brightnessIndex + 1) % brightnessCount;
  Serial.print("Brightness: ");
  Serial.println(brightnessMap[brightnessIndex]);
  matrix.setBrightness(brightnessMap[brightnessIndex]);

  if (brightnessIndex == 0) {
    powerOff();
  }
}

void adjustBrightness(bool up) {
  if (up) {
    if (brightnessIndex < brightnessCount - 1) {
      brightnessIndex++;
    }
  }
  else if (brightnessIndex > 0) {
    brightnessIndex--;
  }

  Serial.print("Brightness: ");
  Serial.println(brightnessMap[brightnessIndex]);
  matrix.setBrightness(brightnessMap[brightnessIndex]);
}

void setup() {
  Serial.begin(9600);
  matrix.addLayer(&backgroundLayer);
  matrix.begin();
  matrix.setBrightness(brightnessMap[brightnessIndex]);
  randomFillWorld();

  // Initialize the IR receiver
  irReceiver.enableIRIn();
}

void printCursorPosition() {
  Serial.print("Cursor: ");
  Serial.print(cursorX);
  Serial.print(",");
  Serial.println(cursorY);
}

void handleInput() {
  InputCommand command = readCommand(defaultHoldDelay);

  switch (command) {
    case InputCommand::Palette:
      cyclePalette();
      break;

    case InputCommand::PlayMode:
      isPaused = !isPaused;
      break;

    case InputCommand::BrightnessUp:
      adjustBrightness(true);
      break;

    case InputCommand::BrightnessDown:
      adjustBrightness(false);
      break;

    case InputCommand::CycleBrightness:
      cycleBrightness();
      break;

    case InputCommand::Power:
      powerOff();
      break;

    case InputCommand::RandomFillWorld:
      if (isPaused) {
        if (isCleared) {
          randomFillWorld();
        }
        else {
          clearWorld();
        }
      }
      else {
        fill_solid((CRGB*)buffer, NUM_LEDS, CRGB::Black);
        randomFillWorld();
      }
      break;

    case InputCommand::Select:
      if (!isPaused) {
        isPaused = true;
      }
      else {
        // toggle the cell at the cursor position
        boolean alive = !world[cursorX][cursorY].alive;

        world[cursorX][cursorY].alive = alive;
        world[cursorX][cursorY].brightness = alive ? 255 : 0;
        world[cursorX][cursorY].prev = alive;
        world[cursorX][cursorY].hue = 0;
      }
      break;

    case InputCommand::Up:
      if (isPaused) {
        if (cursorY > 0)
          cursorY--;
        printCursorPosition();
      }
      else {
        if (currentDelay > 0)
          currentDelay--;
      }
      break;

    case InputCommand::Down:
      if (isPaused) {
        isPaused = true;
        if (cursorY < kMatrixHeight - 1)
          cursorY++;
        printCursorPosition();
      }
      else {
        if (currentDelay < 255)
          currentDelay++;
      }
      break;

    case InputCommand::Left:
      isPaused = true;
      if (cursorX > 0)
        cursorX--;
      printCursorPosition();
      break;

    case InputCommand::Right:
      isPaused = true;
      if (cursorX < kMatrixWidth - 1)
        cursorX++;
      printCursorPosition();
      break;

    case InputCommand::None:
    default:
      break;
  }
}

void loop() {
  buffer = backgroundLayer.backBuffer();

  handleInput();

  // Display current generation
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      if (blur) {
        buffer[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
      }
      else if (world[i][j].alive) {
        buffer[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
      }
      else {
        buffer[XY(i, j)] = (CRGB)CRGB::Black;
      }
    }
  }

  if (isPaused) {
    backgroundLayer.drawPixel(cursorX, cursorY, CRGB(CRGB::White));
  }
  else {
    isCleared = false;

    // Birth and death cycle
    for (int x = 0; x < kMatrixWidth; x++) {
      for (int y = 0; y < kMatrixHeight; y++) {
        // Default is for cell to stay the same
        if (world[x][y].brightness > 0 && world[x][y].prev == 0)
          world[x][y].brightness *= blurAmount;
        int count = neighbours(x, y);
        if (count == 3 && world[x][y].prev == 0) {
          // A new cell is born
          world[x][y].alive = true;
          world[x][y].hue += 2;
          world[x][y].brightness = 255;
        }
        else if ((count < 2 || count > 3) && world[x][y].prev) {
          // Cell dies
          world[x][y].alive = false;
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
  }

  backgroundLayer.swapBuffers();

  delay(currentDelay);
}
