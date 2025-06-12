#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"


#define TRIG_PIN GPIO_NUM_4
#define ECHO_PIN GPIO_NUM_5

#define RED_PIN  GPIO_NUM_25
#define GREEN_PIN GPIO_NUM_26
#define BLUE_PIN GPIO_NUM_27

#define BUZZER_PIN GPIO_NUM_15

#define LDR_ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36 (A0) → ADC1_CH0

void init_adc() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_11);
}

// Reads the LDR value from the ADC channel
int read_ldr() {
    return adc1_get_raw(LDR_ADC_CHANNEL);
}

float get_distance_cm(){
        
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10);
    gpio_set_level(TRIG_PIN, 0);
    gpio_set_level(TRIG_PIN, 0);

    uint32_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if (esp_timer_get_time() - start_time > 100000) return -1;
    }

    uint32_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if (esp_timer_get_time() - echo_start > 100000) return -1;
    }
    uint32_t echo_end = esp_timer_get_time();

    float duration_us = echo_end - echo_start;
    return (duration_us / 2.0) * 0.0343;
};

void init_pwm_led(gpio_num_t pin, ledc_channel_t channel)
{
    ledc_channel_config_t ledc_channel = {
        .channel    = channel,
        .duty       = 0,
        .gpio_num   = pin,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
    };
    ledc_channel_config(&ledc_channel);
}

void set_rgb_color(uint8_t r, uint8_t g, uint8_t b)
{
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r * 4);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g * 4);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b * 4);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
}

void play_buzzer()
{
    gpio_set_level(BUZZER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(300));
    gpio_set_level(BUZZER_PIN, 0);
}

void app_main(void)
{
    // GPIO direction setup
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(BUZZER_PIN, GPIO_MODE_OUTPUT);

    init_adc();

    // --- LEDC Timer Setup ---
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0
    };
    ledc_timer_config(&ledc_timer);

    // --- RGB Channel Init ---
    init_pwm_led(RED_PIN, LEDC_CHANNEL_0);
    init_pwm_led(GREEN_PIN, LEDC_CHANNEL_1);
    init_pwm_led(BLUE_PIN, LEDC_CHANNEL_2);

    while (1) {
        float distance = get_distance_cm();
        int light = read_ldr();

        ESP_LOGI("Sensor", "Distance: %.2f cm, Light: %d", distance, light);

        if (distance < 10) {
            set_rgb_color(255, 0, 0); // Red
            play_buzzer();
        } else if (distance < 20) {
            set_rgb_color(255, 165, 0); // Orange-ish
            play_buzzer();
        } else {
            set_rgb_color(0, 255, 0); // Green
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }


}