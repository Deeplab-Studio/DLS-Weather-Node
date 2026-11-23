#pragma once

#include <Arduino.h>

// Bu baslik yalnizca ESP32 (klasik) hedefi icin derlenmelidir.
#if !defined(DLS_TARGET_ESP32)
#error "Yanlis variant.h: esp32/variant.h secildi ama hedef ESP32 degil."
#endif

// --- Pin Definitions ---
#define LED_PIN 2

// I2C Pins (Default for ESP32)
#define I2C_SDA 21
#define I2C_SCL 22

// Sensor Power Control (MOSFET)
#define SENSOR_PWR_PIN 4 

// ADC Pins
#define ADC_PIN 34
#define ADC_MULTIPLIER 1.6667 // Divider: 1M + 1.5M -> 2.5/1.5 = 1.666...
#define ADC_SAMPLE_COUNT 8    // Yuksek empedansli bölücü icin ortalama

// Debug configuration
#define DEBUG_MODE false

// --- Weather Sensor Pins ---
#define WDIR_PIN 35
#define WSPEED_PIN 14
#define RAIN_PIN 27
