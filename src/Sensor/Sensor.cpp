#include "Sensor.h"

Sensor::Sensor() {}

void Sensor::begin(TwoWire *wire) {
    _i2c = wire;
    Serial.println("\n[Sensor] Taramasi Baslatiliyor...");

    // --- AIR SENSORS ---
    if (_bme680.begin(0x76)) { // Try 0x76 first
         _foundAirSensor = AIR_BME680;
        Serial.println("[Sensor] BME680 (0x76) Tespit Edildi!");
        _bme680.setTemperatureOversampling(BME680_OS_8X);
        _bme680.setHumidityOversampling(BME680_OS_2X);
        _bme680.setPressureOversampling(BME680_OS_4X);
        _bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
        _bme680.setGasHeater(320, 150);
    } 
    else if (_bme680.begin(0x77)) { // Try 0x77
        _foundAirSensor = AIR_BME680;
        Serial.println("[Sensor] BME680 (0x77) Tespit Edildi!");
        // Settings...
         _bme680.setTemperatureOversampling(BME680_OS_8X);
        _bme680.setHumidityOversampling(BME680_OS_2X);
        _bme680.setPressureOversampling(BME680_OS_4X);
        _bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
        _bme680.setGasHeater(320, 150);
    }
    else if (_sht31.begin(0x44)) {
        _foundAirSensor = AIR_SHT3X;
        Serial.println("[Sensor] SHT3x Tespit Edildi!");
    }
    else if (_shtc3.begin()) {
        _foundAirSensor = AIR_SHTC3;
        Serial.println("[Sensor] SHTC3 Tespit Edildi!");
    }
    else if (_bme280.begin(0x76)) {
        _foundAirSensor = AIR_BME280;
        Serial.println("[Sensor] BME280 Tespit Edildi!");
    }
    else if (_bmp280.begin(0x76)) {
        _foundAirSensor = AIR_BMP280;
        Serial.println("[Sensor] BMP280 Tespit Edildi!");
    }
    else {
        Serial.println("[Sensor] HICBIR HAVA SENSORU BULUNAMADI!");
    }

    // --- LIGHT SENSORS ---
    if (_veml6075.begin()) {
        _foundLightSensor = LIGHT_VEML6075;
         Serial.println("[Sensor] VEML6075 (UV) Tespit Edildi!");
    } else {
        Serial.println("[Sensor] UV sensoru bulunamadi.");
    }
}

bool Sensor::getAirData(AirData &data) {
    data.valid = false;
    
    switch (_foundAirSensor) {
        case AIR_BME680:
            if (_bme680.performReading()) {
                data.temperature = _bme680.temperature;
                data.humidity = _bme680.humidity;
                data.pressure = _bme680.pressure / 100.0F;
                data.gasResistance = _bme680.gas_resistance / 1000.0;
                data.valid = true;
            }
            break;

        case AIR_SHT3X:
            data.temperature = _sht31.readTemperature();
            data.humidity = _sht31.readHumidity();
            data.valid = (!isnan(data.temperature) && !isnan(data.humidity));
            break;

        case AIR_SHTC3:
            sensors_event_t h, t;
            _shtc3.getEvent(&h, &t);
            data.temperature = t.temperature;
            data.humidity = h.relative_humidity;
            data.valid = true;
            break;

        case AIR_BME280:
            data.temperature = _bme280.readTemperature();
            data.humidity = _bme280.readHumidity();
            data.pressure = _bme280.readPressure() / 100.0F;
            data.valid = true;
            break;

        case AIR_BMP280:
            data.temperature = _bmp280.readTemperature();
            data.pressure = _bmp280.readPressure() / 100.0F;
            data.valid = true;
            break;
            
        case AIR_NONE:
        default:
            return false;
    }

    return data.valid;
}

bool Sensor::getLightData(LightData &data) {
    data.valid = false;

    switch (_foundLightSensor) {
        case LIGHT_VEML6075:
            data.uva = _veml6075.readUVA();
            data.uvb = _veml6075.readUVB();
            data.uvIndex = _veml6075.readUVI();
            data.valid = true;
            break;
            
        case LIGHT_NONE:
        default:
            return false;
    }
    
    return data.valid;
}
