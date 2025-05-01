#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/adc.h>
#include <arduinoFFT.h>
#include <FastLED.h>

#define I2S_SAMPLE_RATE   44100
#define I2S_READ_LEN      1024
#define ADC_INPUT_CHANNEL ADC1_CHANNEL_0  // GPIO36 on most ESP32 boards

uint8_t *i2s_read_buff;
int total_bytes_read = 0;
unsigned long start_time;

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
    i2s_set_adc_mode(ADC_UNIT_1, ADC_INPUT_CHANNEL);
    i2s_adc_enable(I2S_NUM_0);
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Give time for serial to connect

    setup_i2s_adc();
    i2s_read_buff = (uint8_t *)calloc(I2S_READ_LEN, sizeof(uint8_t));
    start_time = micros();
}

void loop() {
    size_t bytes_read;
    i2s_read(I2S_NUM_0, (void *)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);
    total_bytes_read += bytes_read;

    unsigned long now = micros();
    float seconds = (now - start_time) / 1e6;
    float samples = total_bytes_read / 2.0;
    float sampling_rate = samples / seconds;

    Serial.printf("Sampling rate: %.2f Hz\n", sampling_rate);

    // Example FFT use point: fill real[] and imag[] arrays here

    //delay(250); // Don't spam the Serial monitor
}
