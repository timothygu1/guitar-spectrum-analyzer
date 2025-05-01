#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define I2S_SAMPLE_RATE   44100
#define I2S_READ_LEN      1024
#define ADC_INPUT_CHANNEL ADC1_CHANNEL_0  // GPIO36 on most ESP32 boards

void setup_i2s_adc()
{
    // Configure I2S to use ADC
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,  // ADC is single channel
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = 0, // Default interrupt priority
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

extern "C" void app_main(void)
{
    setup_i2s_adc();

    uint8_t *i2s_read_buff = (uint8_t *)calloc(I2S_READ_LEN, sizeof(uint8_t));

    while (1) {
        size_t bytes_read;
        i2s_read(I2S_NUM_0, (void *)i2s_read_buff, I2S_READ_LEN, &bytes_read, portMAX_DELAY);

        printf("Read %d bytes: ", bytes_read);
        for (int i = 0; i < 16 && i < bytes_read; i++) {
            printf("%02X ", i2s_read_buff[i]);
        }
        printf("\n");

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    free(i2s_read_buff);
    i2s_adc_disable(I2S_NUM_0);
    i2s_driver_uninstall(I2S_NUM_0);
}