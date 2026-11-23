#pragma once

#include <Arduino.h>

// Bu baslik yalnizca ESP32-S2 hedefi icin derlenmelidir.
#if !defined(DLS_TARGET_ESP32S2)
#error "Yanlis variant.h: esp32s2/variant.h secildi ama hedef ESP32-S2 degil."
#endif

// --- Pin Definitions (LOLIN/WEMOS S2 Mini) ---
#define LED_PIN 15        // Kart uzerindeki tek renkli LED (board tanimiyla ayni)

// I2C Pins (S2 Mini varsayilanlari: SDA 33 / SCL 35)
#define I2C_SDA 33
#define I2C_SCL 35

// Sensor Power Control (MOSFET)
#define SENSOR_PWR_PIN 18

// ADC Pins (S2: ADC1 = GPIO1..GPIO10, kart uzerinde A0..A5 = GPIO1..5)
#define ADC_PIN 1             // Batarya bolucu, ADC1_CH0 (A0)
#define ADC_MULTIPLIER 1.6667 // Divider: 1M + 1.5M -> 2.5/1.5 = 1.666...
#define ADC_SAMPLE_COUNT 8

// --- Weather Sensor Pins (ruzgar/yagmur) ---
#define WDIR_PIN 2            // ADC1_CH1 (A1), 8 kademeli ruzgar yonu
#define WSPEED_PIN 16         // Anemometre reed switch (kesme)
#define RAIN_PIN 17           // Yagmur kepce reed switch (kesme)

// Debug configuration
#define DEBUG_MODE false
