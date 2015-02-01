#include<SmartMatrix_32x32.h>
#include<FastLED.h>

#define NUM_LEDS (MATRIX_WIDTH * MATRIX_HEIGHT)

CRGB leds[NUM_LEDS];

CRGBPalette16 currentPalette = RainbowColors_p;

class Cell {
public:
byte alive : 
  1;
byte prev : 
  1;
byte hue: 
  6;  
  byte brightness;
};

Cell world[MATRIX_WIDTH][MATRIX_HEIGHT];
long density = 50;
int generation = 0;

void setup() {
  LEDS.addLeds<SMART_MATRIX>(leds, NUM_LEDS);
  LEDS.setBrightness(96);
}

void loop() {
  if (generation == 0) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);

    randomFillWorld();
  }

  // Display current generation
  for (int i = 0; i < MATRIX_WIDTH; i++) {
    for (int j = 0; j < MATRIX_HEIGHT; j++) {
      leds[XY(i, j)] = ColorFromPalette(currentPalette, world[i][j].hue * 4), world[i][j].brightness);
    }
  }

  // Birth and death cycle
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      // Default is for cell to stay the same
      if (world[x][y].brightness > 0 && world[x][y].prev == 0)
        world[x][y].brightness *= 0.9;
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
  LEDS.delay(60);
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

