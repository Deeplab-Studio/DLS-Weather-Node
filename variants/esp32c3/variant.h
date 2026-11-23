#pragma once

#include <Arduino.h>

// Bu baslik yalnizca ESP32-C3 hedefi icin derlenmelidir.
#if !defined(DLS_TARGET_ESP32C3)
#error "Yanlis variant.h: esp32c3/variant.h secildi ama hedef ESP32-C3 degil."
#endif

// --- Pin Definitions ---
#define LED_PIN -1

// I2C Pins (Default for ESP32C3)
#define I2C_SDA 8
#define I2C_SCL 9

// Sensor Power Control (MOSFET)
#define SENSOR_PWR_PIN 10

// ADC Pins
#define ADC_PIN 3 // GPIO 3 is often ADC1_CH3 on C3 Super Mini
#define ADC_MULTIPLIER 1.6667 // Divider: 1M + 1.5M -> 2.5/1.5 = 1.666...
#define ADC_SAMPLE_COUNT 8    // Yuksek empedansli bölücü icin ortalama

// Debug configuration
#define DEBUG_MODE false
