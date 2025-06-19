# 🌊 Flood Detection System with ESP32

A smart flood detection system using an **ultrasonic sensor**, **RGB LED**, **buzzer**, and **LDR** (Light Dependent Resistor) to detect water level, indicate danger levels, and differentiate between day and night. Data is transmitted in real-time to **ThingSpeak** via WiFi for monitoring.

---

## 📌 Project Information

- **Project Title**: Flood Detection System with ESP32  
- **Created On**: June 18, 2025  
- **Authors**:  
  - NHEM Darayut  
  - HENG Dararithy  
  - PHENG Pheareakboth  
  - HUN Noradihnaro  

- **Platform**: Keyestudio ESP32 Dev Board (38 pins)  
- **Framework**: ESP-IDF v5.4.0 (VS Code Extension 1.9.1)  
- **WiFi**: ThingSpeak API Integration for data logging

---

## 🧠 Features

- 📏 **Ultrasonic Sensor (HC-SR04)** to measure water level.
- 🌈 **RGB LED** to indicate severity:
  - 🔴 Red: Danger (very close)
  - 🟡 Yellow: Warning (moderately close)
  - 🟢 Green: Safe (far)
- 🔊 **Buzzer** to alert during warning and danger levels.
- 🌙 **LDR Sensor** to detect ambient light (day/night).
- ☁️ **WiFi Connection** to send data to **ThingSpeak**.
- 📈 Real-time cloud-based logging and visualization of:
  - Water level
  - LED state
  - Light condition (day/night)

---

## 📦 Hardware Requirements

| Component          | Quantity |
|--------------------|----------|
| ESP32 Dev Board    | 1        |
| HC-SR04 Sensor     | 1        |
| RGB LED (Common Cathode) | 1  |
| Buzzer             | 1        |
| LDR Module         | 1        |
| Resistors, Breadboard, Jumper Wires | as needed |

---

## 🔌 Pin Configuration

| Component     | ESP32 Pin     |
|---------------|---------------|
| HC-SR04 Trig  | GPIO 4        |
| HC-SR04 Echo  | GPIO 5        |
| RGB Red       | GPIO 25       |
| RGB Green     | GPIO 26       |
| RGB Blue      | GPIO 27       |
| Buzzer        | GPIO 15       |
| LDR (Digital) | GPIO 34       |

---

## ⚙️ PWM Configuration

- **PWM Frequency**: 5 kHz  
- **Resolution**: 8-bit (0–255)  
- **Buzzer Frequency**: 2 kHz (tone)  
- **LEDC Channels**:
  - Red: Channel 0
  - Green: Channel 1
  - Blue: Channel 2
  - Buzzer: Channel 3

---

## 🌐 WiFi & ThingSpeak

Update your WiFi credentials and ThingSpeak Write API key in `main.c`:

```c
#define WIFI_SSID     "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
#define API_KEY       "YourThingSpeakAPIKey"
