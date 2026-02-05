#pragma once

#include <Arduino.h>

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

// Debug configuration
#define DEBUG_MODE true
