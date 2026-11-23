#pragma once

#include <Arduino.h>

// Bu baslik yalnizca ESP32-S3 hedefi icin derlenmelidir. Yanlis variant.h
// include yoluna girerse derleme burada bilerek durur.
#if !defined(DLS_TARGET_ESP32S3)
#error "Yanlis variant.h: esp32s3/variant.h secildi ama hedef ESP32-S3 degil."
#endif

// --- Pin Definitions (ESP32-S3 SuperMini varsayilanlari) ---
// Bazi S3 kartlarinda GPIO48 adreslenebilir (WS2812) RGB LED'dir.
#define LED_PIN 48
#define LED_IS_RGB 1

// I2C Pins
#define I2C_SDA 4
#define I2C_SCL 5

// Sensor Power Control (MOSFET)
#define SENSOR_PWR_PIN 6

// ADC Pins (S3: ADC1 = GPIO1..GPIO10)
#define ADC_PIN 1             // Batarya bolucu, ADC1_CH0
#define ADC_MULTIPLIER 1.6667 // Divider: 1M + 1.5M -> 2.5/1.5 = 1.666...
#define ADC_SAMPLE_COUNT 8

// --- Weather Sensor Pins (ruzgar/ygmur) ---
#define WDIR_PIN 2            // ADC1_CH1, 8 kademeli ruzgar yonu
#define WSPEED_PIN 7          // Anemometre reed switch (kesme)
#define RAIN_PIN 8            // Yagmur kepce reed switch (kesme)

// Debug configuration
#define DEBUG_MODE false
