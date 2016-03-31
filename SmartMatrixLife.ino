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
#include "Cell.h"

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
const uint8_t kIndexedLayerOptions = (SM_INDEXED_OPTIONS_NONE);

SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, kPanelType, kMatrixOptions);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(backgroundLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kBackgroundLayerOptions);
SMARTMATRIX_ALLOCATE_INDEXED_LAYER(indexedLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, kIndexedLayerOptions);

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)

rgb24 *buffer;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

boolean isPaused = false;

boolean isCleared = false;

uint8_t cursorX = kMatrixWidth / 2;
uint8_t cursorY = kMatrixHeight / 2;

uint8_t brightnessIndex = 2;
const uint8_t brightnessMap[] = { 16, 32, 64, 128, 255 };
const uint8_t brightnessCount = ARRAY_SIZE(brightnessMap);

const uint8_t delayMap[] = { 255, 224, 192, 160, 128, 96, 64, 32, 16, 8, 0 };
const uint8_t delayCount = ARRAY_SIZE(delayMap);
uint8_t delayIndex = delayCount - 2;
elapsedMillis sinceLastUpdate = 0;

const float blurMap[] = { 0.0f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 0.92f, 0.94f, 0.96f, 0.98f, 0.99f, 1.0f };
const uint8_t blurCount = ARRAY_SIZE(blurMap);
uint8_t blurIndex = 3;

uint8_t cyclesWithoutBirth = 0;

// automatically reset the world if there are no new births for a while
boolean autoReset = false;

boolean showingIndicator = false;
elapsedMillis sinceIndicatorShown = 0;
char indicatorText[10];

const uint16_t indicatorDuration = 1000;

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

char const * paletteNames[] = {
  "Rainbow",
  "Rnbw Strp",
  "Ocean",
  "Cloud",
  "Forest",
  "Party",
  "Heat",
  "Lava",
};

uint8_t currentPaletteIndex = 0;

const uint8_t paletteCount = ARRAY_SIZE(palettes);

CRGBPalette16 currentPalette = palettes[currentPaletteIndex];

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

  showingIndicator = true;
  sinceIndicatorShown = 0;
  sprintf(indicatorText, paletteNames[currentPaletteIndex]);
}

void togglePaused() {
  isPaused = !isPaused;

  showingIndicator = isPaused;
  sinceIndicatorShown = 0;
  sprintf(indicatorText, "Paused");
}

void adjustSpeed(bool up) {
  if (up) {
    if (delayIndex < delayCount - 1) {
      delayIndex++;
    }
  }
  else if (delayIndex > 0) {
    delayIndex--;
  }

  showingIndicator = true;
  sinceIndicatorShown = 0;
  uint8_t level = ((float) delayIndex / (float) (delayCount - 1)) * 100;
  if (level < 1) level = 1;
  sprintf(indicatorText, "Spd:%3d%%", level); // 100%
}

void adjustBlur(bool up) {
  if (up) {
    if (blurIndex < blurCount - 1) {
      blurIndex++;
    }
  }
  else if (blurIndex > 0) {
    blurIndex--;
  }

  showingIndicator = true;
  sinceIndicatorShown = 0;
  uint8_t level = blurMap[blurIndex] * 100.0f;
  sprintf(indicatorText, "Blr:%3d%%", level); // 100%
}

void showBrightnessIndicator() {
  showingIndicator = true;
  sinceIndicatorShown = 0;
  uint8_t level = ((float) brightnessIndex / (float) (brightnessCount - 1)) * 100;
  if (level < 1) level = 1;
  sprintf(indicatorText, "%3d%%", level); // 100%
}

void cycleBrightness() {
  brightnessIndex = (brightnessIndex + 1) % brightnessCount;
  Serial.print("Brightness: ");
  Serial.println(brightnessMap[brightnessIndex]);
  matrix.setBrightness(brightnessMap[brightnessIndex]);

  if (brightnessIndex == 0) {
    powerOff();
  }
  else {
    showBrightnessIndicator();
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

  showBrightnessIndicator();
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
      togglePaused();
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
      if (isPaused) {
        // toggle the cell at the cursor position
        boolean alive = !world[cursorX][cursorY].alive;

        world[cursorX][cursorY].alive = alive;
        world[cursorX][cursorY].brightness = alive ? 255 : 0;
        world[cursorX][cursorY].prev = alive;
        world[cursorX][cursorY].hue = 0;
      }
      else {
        togglePaused();
      }
      break;

    case InputCommand::Up:
      if (isPaused) {
        if (cursorY > 0)
          cursorY--;
        printCursorPosition();
      }
      else {
        adjustSpeed(true);
      }
      break;

    case InputCommand::Down:
      if (isPaused) {
        if (cursorY < kMatrixHeight - 1)
          cursorY++;
        printCursorPosition();
      }
      else {
        adjustSpeed(false);
      }
      break;

    case InputCommand::Left:
      if (isPaused) {
        if (cursorX > 0)
          cursorX--;
        printCursorPosition();
      }
      else {
        adjustBlur(false);
      }
      break;

    case InputCommand::Right:
      if (isPaused) {
        if (cursorX < kMatrixWidth - 1)
          cursorX++;
        printCursorPosition();
      }
      else {
        adjustBlur(true);
      }
      break;

    case InputCommand::None:
    default:
      break;
  }
}

void drawCursor() {
  backgroundLayer.drawPixel(cursorX - 1, cursorY, CRGB(CRGB::White));
  backgroundLayer.drawPixel(cursorX + 1, cursorY, CRGB(CRGB::White));
  backgroundLayer.drawPixel(cursorX, cursorY - 1, CRGB(CRGB::White));
  backgroundLayer.drawPixel(cursorX, cursorY + 1, CRGB(CRGB::White));
}

void setup() {
  Serial.begin(9600);
  matrix.addLayer(&backgroundLayer);
  matrix.addLayer(&indexedLayer);
  matrix.begin();
  matrix.setBrightness(brightnessMap[brightnessIndex]);

  backgroundLayer.enableColorCorrection(true);

  indexedLayer.setFont(font3x5);
  indexedLayer.setIndexedColor(1, CRGB(CRGB::White));
  indexedLayer.enableColorCorrection(true);
  indexedLayer.setIndexedColor(1, {255, 255, 255});

  randomFillWorld();

  // Initialize the IR receiver
  irReceiver.enableIRIn();
}

void loop() {
  buffer = backgroundLayer.backBuffer();

  indexedLayer.fillScreen(0);

  handleInput();

  // Display current generation
  for (int i = 0; i < kMatrixWidth; i++) {
    for (int j = 0; j < kMatrixHeight; j++) {
      buffer[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4, world[i][j].brightness);
    }
  }

  if (isPaused) {
    drawCursor();
  }
  else {
    if (sinceLastUpdate >= delayMap[delayIndex]) {
      isCleared = false;

      boolean newBirth = false;

      // Birth and death cycle
      for (int x = 0; x < kMatrixWidth; x++) {
        for (int y = 0; y < kMatrixHeight; y++) {
          // Default is for cell to stay the same
          if (world[x][y].brightness > 0 && world[x][y].prev == 0)
            world[x][y].brightness *= blurMap[blurIndex];
          int count = neighbours(x, y);
          if (count == 3 && world[x][y].prev == 0) {
            // A new cell is born
            world[x][y].alive = true;
            if (world[x][y].hue < 60)
              world[x][y].hue++;
            world[x][y].brightness = 255;
            newBirth = true;
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

      if (!newBirth && autoReset) {
        if (cyclesWithoutBirth++ > 250) {
          randomFillWorld();
          cyclesWithoutBirth = 0;
        }
      }

      sinceLastUpdate = 0;
    }
  }

  if (showingIndicator && sinceIndicatorShown < indicatorDuration) {
    indexedLayer.drawString(0, 0, 1, indicatorText);
  }
  else {
    showingIndicator = false;
  }

  indexedLayer.swapBuffers();

  backgroundLayer.swapBuffers();
}
