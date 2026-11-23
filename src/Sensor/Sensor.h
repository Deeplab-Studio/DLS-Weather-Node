#pragma once

#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include <Adafruit_SHTC3.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_VEML6075.h>

// Sentinel: "olcum yok". NaN kullanilir; boylece API katmani ve ekran
// tek bir kural ile (isfinite) ayirt eder.
#define SENSOR_NAN (NAN)

struct AirData {
    float temperature = NAN;
    float humidity = NAN;
    float pressure = NAN;      // hPa
    float gasResistance = NAN; // kOhm (BME680)
    bool valid = false;
};

struct LightData {
    float uvIndex = NAN;
    float uva = NAN;
    float uvb = NAN;
    bool valid = false;
};

struct WindData {
    float speed = NAN;     // m/s
    float direction = NAN; // derece
    bool valid = false;
};

struct RainData {
    float rate = NAN;  // mm/saat
    float daily = NAN; // mm (gunluk toplam)
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
    bool getWindData(WindData &data);
    bool getRainData(RainData &data);

    // Gun degisiminde gunluk yagis sayacini sifirlar (NTP epoch gun).
    // Donen deger: yeni gunluk toplam (mm) - NVS'e yazmak icin.
    float beginRainDay(int epochDay);
    // Yeniden baslatma/uyku sonrasi gunluk toplamin geri yuklenmesi.
    void restoreRainDaily(float mm);

    // Donanim var mi?
    bool hasAir() const { return _foundAirSensor != AIR_NONE; }
    bool hasLight() const { return _foundLightSensor != LIGHT_NONE; }
    static bool hasWindRain();

    SensorTypeAir getFoundAirSensor() const { return _foundAirSensor; }
    SensorTypeLight getFoundLightSensor() const { return _foundLightSensor; }
    const char *getAirSensorName() const;
    const char *getLightSensorName() const;

private:
    TwoWire *_i2c = nullptr;

    SensorTypeAir _foundAirSensor = AIR_NONE;
    SensorTypeLight _foundLightSensor = LIGHT_NONE;

    float _rainDailyBase = 0.0f; // uygulama basinda geri yuklenen toplam
    int _rainDay = -1;

    Adafruit_BME680 _bme680;
    Adafruit_BME280 _bme280;
    Adafruit_BMP280 _bmp280;
    Adafruit_SHTC3 _shtc3;
    Adafruit_SHT31 _sht31;
    Adafruit_VEML6075 _veml6075;
};
