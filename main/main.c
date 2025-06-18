#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/adc.h"

// Ultrasonic pins
#define TRIG_PIN GPIO_NUM_4
#define ECHO_PIN GPIO_NUM_5

// RGB LED PWM pins
#define RED_PIN    GPIO_NUM_25
#define GREEN_PIN  GPIO_NUM_26
#define BLUE_PIN   GPIO_NUM_27

#define PWM_FREQ       5000     // 5 kHz
#define PWM_RESOLUTION LEDC_TIMER_8_BIT  // 8-bit = duty 0–255

// LEDC channels for RGB
#define RED_CHANNEL     LEDC_CHANNEL_0
#define GREEN_CHANNEL   LEDC_CHANNEL_1
#define BLUE_CHANNEL    LEDC_CHANNEL_2

// Buzzer pin (PWM capable)
#define BUZZER_PIN GPIO_NUM_15

#define BUZZER_CHANNEL  LEDC_CHANNEL_3
#define BUZZER_FREQ     2000  // 2 kHz tone

#define LDR_GPIO GPIO_NUM_34  // DO pin connected here

static const char *TAG = "ULTRASONIC_RGB_BUZZER";

void ldr_init() {
    gpio_set_direction(LDR_GPIO, GPIO_MODE_INPUT);
}

void set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, red);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, RED_CHANNEL);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, green);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, blue);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL);
}

void rgb_pwm_init() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t red_channel = {
        .channel    = RED_CHANNEL,
        .duty       = 0,
        .gpio_num   = RED_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
    };
    ledc_channel_config(&red_channel);

    ledc_channel_config_t green_channel = red_channel;
    green_channel.channel = GREEN_CHANNEL;
    green_channel.gpio_num = GREEN_PIN;
    ledc_channel_config(&green_channel);

    ledc_channel_config_t blue_channel = red_channel;
    blue_channel.channel = BLUE_CHANNEL;
    blue_channel.gpio_num = BLUE_PIN;
    ledc_channel_config(&blue_channel);
}

void buzzer_init() {
    ledc_timer_config_t buzzer_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = BUZZER_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buzzer_timer);

    ledc_channel_config_t buzzer_channel = {
        .channel    = BUZZER_CHANNEL,
        .duty       = 0,  // start silent
        .gpio_num   = BUZZER_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_1
    };
    ledc_channel_config(&buzzer_channel);
}


void buzzer_beep(uint32_t beep_ms, uint32_t pause_ms) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 128);  // 50% duty
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(beep_ms));

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 0);    // silent
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(pause_ms));
}


void ultrasonic_init() {
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_level(TRIG_PIN, 0);
    ESP_LOGI(TAG, "Ultrasonic initialized");
    vTaskDelay(pdMS_TO_TICKS(100));
}

float ultrasonic_get_distance_cm() {
    gpio_set_level(TRIG_PIN, 0);
    esp_rom_delay_us(2);
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(5);
    gpio_set_level(TRIG_PIN, 0);

    uint64_t start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - start) > 30000) return -1;
    }

    uint64_t echo_start = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - echo_start) > 30000) return -1;
    }

    uint64_t echo_end = esp_timer_get_time();
    float distance_cm = (echo_end - echo_start) * 0.0343 / 2.0;
    return distance_cm;
}

int read_ldr() {
    int value = gpio_get_level(LDR_GPIO);  // 0 = DARK, 1 = LIGHT
    ESP_LOGI(TAG, "LDR Digital Output: %d", value);
    return value;
}

void app_main() {
    ldr_init();
    rgb_pwm_init();
    buzzer_init();
    ultrasonic_init();

    while (1) {
        int ldr_value = read_ldr();
        bool is_day = (ldr_value == 0);  // HIGH = more light

        if (is_day) {
            ESP_LOGI(TAG, "It is DAY");
        } else {
            ESP_LOGI(TAG, "It is NIGHT");
        }
        float distance = ultrasonic_get_distance_cm();

        if (distance < 0) {
            ESP_LOGW(TAG, "Failed to read distance");
            set_rgb(0, 0, 255);  // Blue = error
            gpio_set_level(BUZZER_PIN, 0);  // Buzzer off
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            ESP_LOGI(TAG, "Distance: %.2f cm", distance);
            if (distance < 3.0) {
                set_rgb(255, 0, 0);     // Red = very close
                // Fast beep: 150ms on, 150ms off
                buzzer_beep(150, 150);
            } else if (distance < 4.0) {
                set_rgb(255, 255, 0);   // Yellow = medium
                // Slow beep: 500ms on, 500ms off
                buzzer_beep(500, 500);
            } else {
                set_rgb(0, 255, 0);     // Green = far/safe
                ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 0);
                ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);

                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}
