#pragma once

#include <Arduino.h>

// Bu baslik yalnizca ESP32-S3 DevKit hedefi icin derlenmelidir.
#if !defined(DLS_TARGET_ESP32S3_DEVKIT)
#error "Yanlis variant.h: esp32s3_devkit/variant.h secildi ama hedef ESP32-S3 DevKit degil."
#endif

// --- Pin Definitions (ESP32-S3-DevKitC-1) ---
#define LED_PIN 48  // Kart uzerindeki WS2812 (DevKitC-1 v1.1)
#define LED_IS_RGB 1

// I2C Pins (DevKitC-1 icin yaygin kullanim)
#define I2C_SDA 8
#define I2C_SCL 9

// Sensor Power Control (MOSFET)
#define SENSOR_PWR_PIN 7

// ADC Pins (S3: ADC1 = GPIO1..GPIO10)
#define ADC_PIN 1             // Batarya bolucu, ADC1_CH0
#define ADC_MULTIPLIER 1.6667 // Divider: 1M + 1.5M -> 2.5/1.5 = 1.666...
#define ADC_SAMPLE_COUNT 8

// --- Weather Sensor Pins (ruzgar/yagmur) ---
#define WDIR_PIN 2            // ADC1_CH1, 8 kademeli ruzgar yonu
#define WSPEED_PIN 3          // Anemometre reed switch (kesme)
#define RAIN_PIN 4            // Yagmur kepce reed switch (kesme)

// Debug configuration
#define DEBUG_MODE false
