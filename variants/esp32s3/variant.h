#pragma once

#include <Arduino.h>

// --- Pin Definitions ---
// ESP32-S3 usually has RGB LED or similar, defining generic
#define LED_PIN 48 // Common built-in LED for S3 DevKit

// I2C Pins
#define I2C_SDA 4
#define I2C_SCL 5

// Sensor Power Control (MOSFET)
#define SENSOR_PWR_PIN 6

// ADC Pins
#define ADC_PIN 1 // GPIO 1 is ADC1_CH0 on S3
#define ADC_MULTIPLIER 1.6667 // Divider: 1M + 1.5M -> 2.5/1.5 = 1.666...

// Debug configuration
#define DEBUG_MODE false
