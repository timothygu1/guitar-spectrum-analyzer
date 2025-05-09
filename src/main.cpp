#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/adc.h>
#include <arduinoFFT.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "patterns.h"
#include "ble.h"

#define I2S_SAMPLE_RATE   44100
#define I2S_READ_LEN      1024
#define AMPLITUDE         120
#define LED_OUTPUT_PIN    13
#define MAX_MILLIAMPS     1000

const uint8_t kMatrixWidth = 16;
const uint8_t kMatrixHeight = 16;

// FFT
unsigned int sampling_period_us;
byte peak[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};              // The length of these arrays must be >= NUM_BANDS
int oldBarHeights[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int bandValues[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double vReal[I2S_READ_LEN];
double vImag[I2S_READ_LEN];
unsigned long newTime;
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, I2S_READ_LEN, I2S_SAMPLE_RATE);

// LED palettes
CRGB leds[NUM_LEDS];
DEFINE_GRADIENT_PALETTE( purple_gp ) {
  0,   0, 212, 255,   //blue
255, 179,   0, 255 }; //purple
DEFINE_GRADIENT_PALETTE( outrun_gp ) {
  0, 141,   0, 100,   //purple
127, 255, 192,   0,   //yellow
255,   0,   5, 255 };  //blue
DEFINE_GRADIENT_PALETTE( greenblue_gp ) {
  0,   0, 255,  60,   //green
 64,   0, 236, 255,   //cyan
128,   0,   5, 255,   //blue
192,   0, 236, 255,   //cyan
255,   0, 255,  60 }; //green
DEFINE_GRADIENT_PALETTE( redyellow_gp ) {
  0,   200, 200,  200,   //white
 64,   255, 218,    0,   //yellow
128,   231,   0,    0,   //red
192,   255, 218,    0,   //yellow
255,   200, 200,  200 }; //white
CRGBPalette16 purplePal = purple_gp;
CRGBPalette16 outrunPal = outrun_gp;
CRGBPalette16 greenbluePal = greenblue_gp;
CRGBPalette16 heatPal = redyellow_gp;
uint8_t colorTimer = 0;
bool autoChangePatterns = true;
int mode = 0;

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, kMatrixWidth, kMatrixHeight,
  NEO_MATRIX_TOP        + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS       + NEO_MATRIX_ZIGZAG +
  NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS);

// I2S
uint8_t *i2s_read_buff;
int total_bytes_read = 0;
//unsigned long start_time; // Debug

void setup_i2s_adc()
{
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0,
        .dma_buf_count = 4,
        .dma_buf_len = 1024,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_adc_mode(ADC_UNIT_1, ADC1_CHANNEL_7);
    i2s_adc_enable(I2S_NUM_0);
}

void setup() {
    Serial.begin(115200);

    FastLED.addLeds<CHIPSET, LED_OUTPUT_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
    FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();

    setup_i2s_adc();
    i2s_read_buff = (uint8_t *)calloc(I2S_READ_LEN, sizeof(uint8_t));
    bleInit();

    //start_time = micros(); // Debug
}

void changeMode() {
  autoChangePatterns = false;
  mode = (mode + 1) % 6;
}

void startAutoMode() {
  autoChangePatterns = true;
}

void loop() {

    FastLED.clear();
    
    // Reset bandValues[]
    for (int i = 0; i<NUM_BANDS; i++){
      bandValues[i] = 0;
    }

    size_t bytes_read;
    i2s_read(I2S_NUM_0, (void *)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

    // ----- DEBUG INFO -------
    /*
    total_bytes_read += bytes_read;
    unsigned long now = micros();
    float seconds = (now - start_time) / 1e6;
    float samples = total_bytes_read / 2.0;
    float sampling_rate = samples / seconds;
    Serial.printf("Sampling rate: %.2f Hz\n", sampling_rate);
    */
    
    // Convert to voltage
    for (int i = 0; i < I2S_READ_LEN; i++) {
      double voltage = ((double)i2s_read_buff[i] / 4095.0) * 3.3;
      vReal[i] = voltage;
      vImag[i] = 0;
    }
        
    //Compute FFT
    FFT.dcRemoval();
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();
    
    // Analyse FFT results
    for (int i = 2; i < (I2S_READ_LEN/2); i++){    // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
      if (vReal[i] > NOISE) {
      if (i<=12 )           bandValues[15]  += (int)vReal[i];
      if (i>12   && i<=13  ) bandValues[14]  += (int)vReal[i];
      if (i>13   && i<=15  ) bandValues[13]  += (int)vReal[i];
      if (i>15   && i<=16  ) bandValues[12]  += (int)vReal[i];
      if (i>16   && i<=18  ) bandValues[11]  += (int)vReal[i];
      if (i>18   && i<=19  ) bandValues[10]  += (int)vReal[i];
      if (i>19   && i<=21  ) bandValues[9]  += (int)vReal[i];
      if (i>21   && i<=23  ) bandValues[8]  += (int)vReal[i];
      if (i>23   && i<=25  ) bandValues[7]  += (int)vReal[i];
      if (i>25   && i<=28  ) bandValues[6]  += (int)vReal[i];
      if (i>28   && i<=31  ) bandValues[5]  += (int)vReal[i];
      if (i>31   && i<=34  ) bandValues[4]  += (int)vReal[i];
      if (i>34   && i<=37  ) bandValues[3]  += (int)vReal[i];
      if (i>37   && i<=40  ) bandValues[2]  += (int)vReal[i];
      if (i>40   && i<=44  ) bandValues[1]  += (int)vReal[i];
      if (i>44 && i<=100  ) bandValues[0]  += (int)vReal[i];
      }
    }

    // Process the FFT data into bar heights
    for (byte band = 0; band < NUM_BANDS; band++) {

    // Scale the bars for the display
    int barHeight = bandValues[band] / AMPLITUDE;
    if (barHeight > TOP) barHeight = TOP;

    // Small amount of averaging between frames
    barHeight = ((oldBarHeights[band] * 1) + barHeight) / 2;

    // Move peak up
    if (barHeight > peak[band]) {
      peak[band] = min(TOP, barHeight);
    }

     // Draw bars
    switch (mode) {
      case 0:
        rainbowBars(band, barHeight);
        break;
      case 1:
        // No bars on this one
        break;
      case 2:
        purpleBars(band, barHeight);
        break;
      case 3:
        centerBars(band, barHeight);
        break;
      case 4:
        changingBars(band, barHeight);
        break;
    }

     // Draw peaks
    switch (mode) {
      case 0:
        whitePeak(band);
        break;
      case 1:
        outrunPeak(band);
        break;
      case 2:
        whitePeak(band);
        break;
      case 3:
        // No peaks
        break;
      case 4:
        // No peaks
        break;
    }

    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
    }

    // Decay peak
    EVERY_N_MILLISECONDS(60) {
      for (byte band = 0; band < NUM_BANDS; band++) {
        if (peak[band] > 0) peak[band] -= 1;
      colorTimer++;
      }
    }

    // Used in some of the patterns
    EVERY_N_MILLISECONDS(10) {
      colorTimer++;
    }

    EVERY_N_SECONDS(7) {
      if (autoChangePatterns) mode = (mode + 1) % 5;
    }

    FastLED.show();
  }


