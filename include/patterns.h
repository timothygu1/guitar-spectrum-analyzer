#pragma once

#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#define COLOR_ORDER       GRB
#define CHIPSET           WS2812B
#define LED_VOLTS         5
#define LED_BRIGHTNESS    40
#define NUM_BANDS         16
#define NOISE             50
#define NUM_LEDS       (kMatrixWidth * kMatrixHeight)
#define BAR_WIDTH      (kMatrixWidth  / (NUM_BANDS - 1))
#define TOP            (kMatrixHeight - 0)
#define SERPENTINE     true

extern const uint8_t kMatrixWidth;
extern const uint8_t kMatrixHeight;
extern FastLED_NeoMatrix *matrix;
extern CRGBPalette16 purplePal;
extern CRGBPalette16 outrunPal;
extern CRGBPalette16 greenbluePal;
extern CRGBPalette16 heatPal;
extern uint8_t colorTimer;
extern uint8_t peak[];

// Pattern functions
void rainbowBars(int band, int barHeight);
void purpleBars(int band, int barHeight);
void changingBars(int band, int barHeight);
void centerBars(int band, int barHeight);
void whitePeak(int band);
void outrunPeak(int band);