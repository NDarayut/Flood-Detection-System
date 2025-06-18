/** 
 * Project: Flood Detection System using ultrasonic sensor HC SR04, RGB LED, Buzzer, and LDR to tell day/night
 * Author: NHEM Darayut, HENG Dararithy, PHENG Pheareakboth, and HUN Noradihnaro
 * Created on: 2025-06-18
 * Hardware platform: Keyestudio ESP32 Dev Board (ESP32 DevKit 38 pins)
 * Software framework: ESP-IDF v5.4.0 (VS Code Extension 1.9.1)
 * Description:
 *  This project uses an ultrasonic sensor to measure distance,
 *  controls an RGB LED via PWM to indicate proximity,
 *  uses a buzzer to provide audio alerts,
 *  and reads an LDR sensor to detect day/night conditions.
 */

/*------------------------------------------------------------------------------
 HEADERS
------------------------------------------------------------------------------*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "nvs_flash.h"

/*------------------------------------------------------------------------------
 PROGRAM CONSTANTS
------------------------------------------------------------------------------*/
// Ultrasonic pins
#define TRIG_PIN     GPIO_NUM_4
#define ECHO_PIN     GPIO_NUM_5

// RGB LED PWM pins
#define RED_PIN      GPIO_NUM_25
#define GREEN_PIN    GPIO_NUM_26
#define BLUE_PIN     GPIO_NUM_27

// PWM Configurations
#define PWM_FREQ       5000                   // 5 kHz PWM frequency
#define PWM_RESOLUTION LEDC_TIMER_8_BIT      // 8-bit PWM resolution (0-255)

// LEDC channels
#define RED_CHANNEL    LEDC_CHANNEL_0
#define GREEN_CHANNEL  LEDC_CHANNEL_1
#define BLUE_CHANNEL   LEDC_CHANNEL_2

// Buzzer pin and config
#define BUZZER_PIN     GPIO_NUM_15
#define BUZZER_CHANNEL LEDC_CHANNEL_3
#define BUZZER_FREQ    2000                   // 2 kHz tone

// LDR input pin
#define LDR_GPIO      GPIO_NUM_34

// WiFi Connection and ThingSpeak API
#define WIFI_SSID "CDR Cambodia"
#define WIFI_PASSWORD "Darey0864"

#define API_KEY "DNXC7HGIKZAR6XO4" // Write API Key from ThingSpeak

/*------------------------------------------------------------------------------
 GLOBAL VARIABLES
------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------
 FUNCTION DECLARATIONS
------------------------------------------------------------------------------*/
void ldr_init(void);
void rgb_pwm_init(void);
void buzzer_init(void);
void ultrasonic_init(void);
void set_rgb(uint8_t red, uint8_t green, uint8_t blue);
void buzzer_beep(uint32_t beep_ms, uint32_t pause_ms);
float ultrasonic_get_distance_cm(void);
int read_ldr(void);

void wifi_init(void);
void send_to_thingspeak(int distance, int led_state, int is_night);

/*------------------------------------------------------------------------------
 MAIN FUNCTION
------------------------------------------------------------------------------*/
void app_main(void) {

    int led_state;

    // Initialize hardware components
    ldr_init();
    rgb_pwm_init();
    buzzer_init();
    ultrasonic_init();

    // Initialize non-volatile storage
    nvs_flash_init();
    // Initialize WiFi
    wifi_init();

    while (1) {
        bool is_night = read_ldr();  // 1 = Dark, 0 = Light
        
        if (!is_night) {
            printf("%s\n", "Daytime detected");
        } 
        else {
            printf("%s\n", "Nighttime detected");
        }

        float distance = ultrasonic_get_distance_cm();

        if (distance < 0) {
            printf("%s\n", "Failed to read distance");
            set_rgb(0, 0, 255);  // Blue = error indicator
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        printf("Distance: %.2f cm\n", distance);

        if (distance < 3.0f) {
            set_rgb(255, 0, 0);     // Red = very close
            buzzer_beep(100, 100);  // Fast beep
            led_state = 1;
        } 
        else if (distance < 4.0f) {
            set_rgb(255, 255, 0);   // Yellow = medium distance
            buzzer_beep(500, 500);  // Slow beep
            led_state = 2;
        } 
        else {
            set_rgb(0, 255, 0);     // Green = safe/far
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 0);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(1000));
            led_state = 3;
        }

        // Send data to ThingSpeak (for example, distance and LDR value)
        send_to_thingspeak((int)distance, led_state, is_night);
        //vTaskDelay(pdMS_TO_TICKS(500)); // Reduce to 500 ms or less to allow faster beeping
    }
}

/*------------------------------------------------------------------------------
 FUNCTION DEFINITIONS
------------------------------------------------------------------------------*/

/**
 * @brief Initialize the LDR GPIO as input.
 */
void ldr_init(void) {
    gpio_set_direction(LDR_GPIO, GPIO_MODE_INPUT);
}

/**
 * @brief Initialize PWM channels for RGB LED.
 */
void rgb_pwm_init(void) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = PWM_RESOLUTION,
        .freq_hz          = PWM_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t base_channel = {
        .duty       = 0,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
    };

    base_channel.channel = RED_CHANNEL;
    base_channel.gpio_num = RED_PIN;
    ledc_channel_config(&base_channel);

    base_channel.channel = GREEN_CHANNEL;
    base_channel.gpio_num = GREEN_PIN;
    ledc_channel_config(&base_channel);

    base_channel.channel = BLUE_CHANNEL;
    base_channel.gpio_num = BLUE_PIN;
    ledc_channel_config(&base_channel);
}

/**
 * @brief Initialize PWM channel for buzzer.
 */
void buzzer_init(void) {
    ledc_timer_config_t buzzer_timer = {
        .speed_mode       = LEDC_HIGH_SPEED_MODE,
        .timer_num        = LEDC_TIMER_1,
        .duty_resolution  = PWM_RESOLUTION,
        .freq_hz          = BUZZER_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&buzzer_timer);

    ledc_channel_config_t buzzer_channel = {
        .channel    = BUZZER_CHANNEL,
        .duty       = 0,
        .gpio_num   = BUZZER_PIN,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_1
    };
    ledc_channel_config(&buzzer_channel);
}

/**
 * @brief Initialize ultrasonic sensor pins.
 */
void ultrasonic_init(void) {
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_level(TRIG_PIN, 0);
    printf("%s\n", "Ultrasonic initialized");
    vTaskDelay(pdMS_TO_TICKS(100));
}

/**
 * @brief Set RGB LED PWM duty cycle.
 * @param red Duty cycle for red (0-255)
 * @param green Duty cycle for green (0-255)
 * @param blue Duty cycle for blue (0-255)
 */
void set_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, RED_CHANNEL, red);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, RED_CHANNEL);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL, green);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, GREEN_CHANNEL);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL, blue);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BLUE_CHANNEL);
}

/**
 * @brief Make the buzzer beep with given beep and pause durations.
 * @param beep_ms Duration of beep in milliseconds
 * @param pause_ms Duration of pause after beep in milliseconds
 */
void buzzer_beep(uint32_t beep_ms, uint32_t pause_ms) {
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 128);  // 50% duty cycle
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(beep_ms));

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(pause_ms));
}

/**
 * @brief Measure distance using ultrasonic sensor.
 * @return Distance in centimeters, or -1 if timeout/error
 */
float ultrasonic_get_distance_cm(void) {
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
    float distance_cm = (echo_end - echo_start) * 0.0343f / 2.0f;
    return distance_cm;
}

/**
 * @brief Read the LDR digital output.
 * @return 1 if light detected, 0 if dark
 */
int read_ldr(void) {
    int value = gpio_get_level(LDR_GPIO);
    printf("LDR Digital Output: %d\n", value);
    return value;
}

/**
 * @brief WiFi initialization function
 * 
 * Configure WiFi APIs and set WiFi connection
 */
void wifi_init(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    printf("WiFi initialization started\n");
}

/**
 * @brief Send data to ThingSpeak function.
 * 
 * Build request to send an integer value to a ThingSpeak channel using the
 * specified API key.
 * 
 * @param value1 Integer value (e.g., sensor data) to send to ThingSpeak field1.
 * @param value2 Integer value to send to ThingSpeak field2.
 */ 
void send_to_thingspeak(int distance, int led_state, int is_night) {
    char url[256];
    snprintf(url, sizeof(url),
        "http://api.thingspeak.com/update?api_key=%s&field1=%d&field2=%d&field3=%d", API_KEY, distance, led_state, is_night);

    esp_http_client_config_t config = {
        .url = url,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
    printf("Data sent to ThingSpeak: field1=%d, field2=%d, field3=%d\n", distance, led_state, is_night);
}