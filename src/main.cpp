#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/adc.h>
#include <arduinoFFT.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#define I2S_SAMPLE_RATE   44100
#define I2S_READ_LEN      1024
#define AMPLITUDE         200
#define ADC_INPUT_CHANNEL ADC1_CHANNEL_0  // GPIO36 on ESP32-WROVER
#define LED_OUTPUT_PIN    13
#define COLOR_ORDER       GRB
#define CHIPSET           WS2812B       // LED strip type
#define MAX_MILLIAMPS     1000
#define LED_VOLTS         5
#define LED_BRIGHTNESS    50
#define NUM_BANDS         16
#define NOISE             10
const uint8_t kMatrixWidth = 16;
const uint8_t kMatrixHeight = 16;
#define NUM_LEDS       (kMatrixWidth * kMatrixHeight)
#define BAR_WIDTH      (kMatrixWidth  / (NUM_BANDS - 1))
#define TOP            (kMatrixHeight - 0)
#define SERPENTINE     true

// Sampling and FFT
unsigned int sampling_period_us;
byte peak[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};              // The length of these arrays must be >= NUM_BANDS
int oldBarHeights[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int bandValues[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
double vReal[I2S_READ_LEN];
double vImag[I2S_READ_LEN];
unsigned long newTime;
ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, I2S_READ_LEN, I2S_SAMPLE_RATE);

// FastLED
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
bool autoChangePatterns = false;
int buttonPushCounter = 0;

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(leds, kMatrixWidth, kMatrixHeight,
  NEO_MATRIX_TOP        + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS       + NEO_MATRIX_ZIGZAG +
  NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ROWS);

// I2S
uint8_t *i2s_read_buff;
int total_bytes_read = 0;
unsigned long start_time;

// void setup_i2s_adc()
// {
//     i2s_config_t i2s_config = {
//         .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
//         .sample_rate = I2S_SAMPLE_RATE,
//         .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
//         .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
//         .communication_format = I2S_COMM_FORMAT_I2S_MSB,
//         .intr_alloc_flags = 0,
//         .dma_buf_count = 4,
//         .dma_buf_len = 1024,
//         .use_apll = false,
//         .tx_desc_auto_clear = false,
//         .fixed_mclk = 0
//     };

//     i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
//     i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT_CHANNEL);
//     i2s_adc_enable(I2S_NUM_0);
// }

void setup() {
    Serial.begin(115200);

    FastLED.addLeds<CHIPSET, LED_OUTPUT_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
    FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTS, MAX_MILLIAMPS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    FastLED.clear();

    // setup_i2s_adc();
    // i2s_read_buff = (uint8_t *)calloc(I2S_READ_LEN, sizeof(uint8_t));
    start_time = micros();
    sampling_period_us = round(1000000 * (1.0 / I2S_SAMPLE_RATE));
}

void changeMode() {
  autoChangePatterns = false;
  buttonPushCounter = (buttonPushCounter + 1) % 6;
}

void startAutoMode() {
  autoChangePatterns = true;
}

// PATTERNS BELOW //

void rainbowBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix->drawPixel(x, y, CHSV((x / BAR_WIDTH) * (255 / NUM_BANDS), 255, 255));
    }
  }
}


void purpleBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix->drawPixel(x, y, ColorFromPalette(purplePal, y * (255 / (barHeight + 1))));
    }
  }
}

void changingBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    for (int y = TOP; y >= TOP - barHeight; y--) {
      matrix->drawPixel(x, y, CHSV(y * (255 / kMatrixHeight) + colorTimer, 255, 255));
    }
  }
}

void centerBars(int band, int barHeight) {
  int xStart = BAR_WIDTH * band;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    if (barHeight % 2 == 0) barHeight--;
    int yStart = ((kMatrixHeight - barHeight) / 2 );
    for (int y = yStart; y <= (yStart + barHeight); y++) {
      int colorIndex = constrain((y - yStart) * (255 / barHeight), 0, 255);
      matrix->drawPixel(x, y, ColorFromPalette(heatPal, colorIndex));
    }
  }
}

void whitePeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix->drawPixel(x, peakHeight, CHSV(0,0,255));
  }
}

void outrunPeak(int band) {
  int xStart = BAR_WIDTH * band;
  int peakHeight = TOP - peak[band] - 1;
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix->drawPixel(x, peakHeight, ColorFromPalette(outrunPal, peakHeight * (255 / kMatrixHeight)));
  }
}

void waterfall(int band) {
  int xStart = BAR_WIDTH * band;
  double highestBandValue = 60000;        // Set this to calibrate your waterfall

  // Draw bottom line
  for (int x = xStart; x < xStart + BAR_WIDTH; x++) {
    matrix->drawPixel(x, 0, CHSV(constrain(map(bandValues[band],0,highestBandValue,160,0),0,160), 255, 255));
  }

  // Move screen up starting at 2nd row from top
  if (band == NUM_BANDS - 1){
    for (int y = kMatrixHeight - 2; y >= 0; y--) {
      for (int x = 0; x < kMatrixWidth; x++) {
        int pixelIndexY = matrix->XY(x, y + 1);
        int pixelIndex = matrix->XY(x, y);
        leds[pixelIndexY] = leds[pixelIndex];
      }
    }
  }
}

void loop() {

    // Reset bandValues[]
    for (int i = 0; i<NUM_BANDS; i++){
      bandValues[i] = 0;
    }
    
      // Sample the audio pin
    for (int i = 0; i < I2S_READ_LEN; i++) {
      newTime = micros();
      vReal[i] = analogRead(36); // A conversion takes about 9.7uS on an ESP32
      Serial.println(analogRead(36));
      vImag[i] = 0;
      while ((micros() - newTime) < sampling_period_us) { /* chill */ }
    }
    //i2s_read(I2S_NUM_0, (void *)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
    //total_bytes_read += bytes_read;

    // ----- DEBUG INFO -------
    /*
    unsigned long now = micros();
    float seconds = (now - start_time) / 1e6;
    float samples = total_bytes_read / 2.0;
    float sampling_rate = samples / seconds;
    Serial.printf("Sampling rate: %.2f Hz\n", sampling_rate);
    */
    
    // 1024 bytes per sample
    // for (int i = 0; i < I2S_READ_LEN; i++) {
    //   double voltage = ((double)i2s_read_buff[i] / 4095.0) * 3.3;
    //   vReal[i] = voltage;
    //   vImag[i] = 0;
    // }
        
    // for (int i = 0; i < 16; i += 2) {
    //     uint16_t sample = i2s_read_buff[i] | (i2s_read_buff[i + 1] << 8);
    //     Serial.printf("Raw bytes: %02X %02X => Sample: %d\n", i2s_read_buff[i], i2s_read_buff[i+1], sample);
    // }
    
    //Compute FFT
    FFT.dcRemoval();
    FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(FFT_FORWARD);
    FFT.complexToMagnitude();

    
    // Analyse FFT results
    for (int i = 2; i < (I2S_READ_LEN/2); i++){    // Don't use sample 0 and only first SAMPLES/2 are usable. Each array element represents a frequency bin and its value the amplitude.
      if (vReal[i] > NOISE) {
        //16 bands, 6kHz top band
        if (i<=2 )           bandValues[0]  += (int)vReal[i];
        if (i>2   && i<=3  ) bandValues[1]  += (int)vReal[i];
        if (i>3   && i<=4  ) bandValues[2]  += (int)vReal[i];
        if (i>4   && i<=5  ) bandValues[3]  += (int)vReal[i];
        if (i>5   && i<=7  ) bandValues[4]  += (int)vReal[i];
        if (i>7   && i<=9  ) bandValues[5]  += (int)vReal[i];
        if (i>9   && i<=12  ) bandValues[6]  += (int)vReal[i];
        if (i>12   && i<=16  ) bandValues[7]  += (int)vReal[i];
        if (i>16   && i<=22  ) bandValues[8]  += (int)vReal[i];
        if (i>22   && i<=29  ) bandValues[9]  += (int)vReal[i];
        if (i>29   && i<=39  ) bandValues[10]  += (int)vReal[i];
        if (i>39   && i<=51  ) bandValues[11]  += (int)vReal[i];
        if (i>51   && i<=69  ) bandValues[12]  += (int)vReal[i];
        if (i>69   && i<=91  ) bandValues[13]  += (int)vReal[i];
        if (i>91   && i<=122  ) bandValues[14]  += (int)vReal[i];
        if (i>122             ) bandValues[15]  += (int)vReal[i];
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

    // -- DEBUG
    
    // Serial.print("Bar Heights: ");
    // for (int i = 0; i < NUM_BANDS; i++) {
    //   int barHeight = bandValues[i] / AMPLITUDE;
    //   Serial.print(barHeight);
    //   Serial.print(i < NUM_BANDS - 1 ? ", " : "\n");
    // }   

     // Draw bars
    switch (buttonPushCounter) {
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
      case 5:
        waterfall(band);
        break;
    }

     // Draw peaks
    switch (buttonPushCounter) {
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
      case 5:
        // No peaks
        break;
    }

    // Save oldBarHeights for averaging later
    oldBarHeights[band] = barHeight;
    }

    // Decay peak
    EVERY_N_MILLISECONDS(60) {
        Serial.printf("Decay tick");  // Debug print
      for (byte band = 0; band < NUM_BANDS; band++) {
        Serial.printf("peak[%d] = %d ", band, peak[band]);
        if (peak[band] > 0) 
        {
          peak[band] -= 1;
          Serial.printf("decayed to  %d",peak[band]);
        }
        Serial.printf("\n");
      }
      colorTimer++;
    }

    // Used in some of the patterns
    EVERY_N_MILLISECONDS(10) {
      colorTimer++;
    }

    EVERY_N_SECONDS(10) {
      if (autoChangePatterns) buttonPushCounter = (buttonPushCounter + 1) % 6;
    }

    FastLED.show();
  }


