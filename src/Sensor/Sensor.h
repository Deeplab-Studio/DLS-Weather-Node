#pragma once

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SHTC3.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_VEML6075.h>

struct AirData {
    float temperature = -999.0;
    float humidity = -999.0;
    float pressure = -999.0;
    float gasResistance = -999.0;
    bool valid = false;
};

struct LightData {
    float uvIndex = -1.0;
    float uva = -1.0;
    float uvb = -1.0;
    bool valid = false;
};

enum SensorTypeAir {
    AIR_NONE,
    AIR_BME680,
    AIR_BME280,
    AIR_BMP280,
    AIR_SHTC3,
    AIR_SHT3X
};

enum SensorTypeLight {
    LIGHT_NONE,
    LIGHT_VEML6075
};

class Sensor {
public:
    Sensor();
    void begin(TwoWire *wire = &Wire);
    
    // Data Readers
    bool getAirData(AirData &data);
    bool getLightData(LightData &data);

    // Getters for detected types
    SensorTypeAir getFoundAirSensor() const { return _foundAirSensor; }
    SensorTypeLight getFoundLightSensor() const { return _foundLightSensor; }

private:
    TwoWire *_i2c;
    
    // Detected Types
    SensorTypeAir _foundAirSensor = AIR_NONE;
    SensorTypeLight _foundLightSensor = LIGHT_NONE;

    // Sensor Objects
    Adafruit_BME680 _bme680;
    Adafruit_BME280 _bme280;
    Adafruit_BMP280 _bmp280;
    Adafruit_SHTC3 _shtc3;
    Adafruit_SHT31 _sht31;
    Adafruit_VEML6075 _veml6075;
};
