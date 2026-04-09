#include "Sensor.h"
#include <variant.h> // Pin definitions are here

// --- Config Constants for Wind & Rain ---
#define RAIN_MM_PER_TICK 0.2794
#define WIND_CM_RADIUS 7.0
#define WIND_FACTOR 0.0218

// --- Interrupt Storage ---
volatile unsigned long windPulses = 0;
volatile unsigned long lastWindPulseTime = 0;
unsigned long lastWindSampleTime = 0;

volatile unsigned long rainTips = 0;
volatile unsigned long dailyRainTips = 0;
volatile unsigned long lastRainTipTime = 0;
volatile unsigned long beforeLastRainTipTime = 0;
unsigned long lastRainSampleTime = 0;

// --- ISR (Interrupt Service Routines) ---
void IRAM_ATTR windSpeedISR() {
    unsigned long now = millis();
    if (now - lastWindPulseTime > 15) { // 15ms debounce filter for reed switch
        windPulses++;
        lastWindPulseTime = now;
    }
}

void IRAM_ATTR rainISR() {
    unsigned long now = millis();
    if (now - lastRainTipTime > 100) { // 100ms debounce
        rainTips++;
        dailyRainTips++;
        beforeLastRainTipTime = lastRainTipTime;
        lastRainTipTime = now;
    }
}

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
        Serial.println("[Sensor] SHT3x (0x44 - SHT30/31/35) Tespit Edildi!");
    }
    else if (_sht31.begin(0x45)) {
        _foundAirSensor = AIR_SHT3X;
        Serial.println("[Sensor] SHT3x (0x45 - SHT30/31/35) Tespit Edildi!");
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

    // --- WIND & RAIN PINS CONFIG ---
#if defined(WDIR_PIN) && defined(WSPEED_PIN) && defined(RAIN_PIN)
    pinMode(WDIR_PIN, INPUT); // ADC pin doesn't strictly need this but good practice
    
    // Most passive weather sensors pull to Ground
    pinMode(WSPEED_PIN, INPUT_PULLUP);
    pinMode(RAIN_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(WSPEED_PIN), windSpeedISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainISR, FALLING);
    
    lastWindSampleTime = millis();
    lastRainSampleTime = millis();
    Serial.println("[Sensor] Ruzgar ve Yagmur kesmeleri baslatildi.");
#endif
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

bool Sensor::getWindData(WindData &data) {
    data.valid = false;
#if defined(WDIR_PIN) && defined(WSPEED_PIN)

    unsigned long now = millis();
    unsigned long deltaMs = now - lastWindSampleTime;
    
    if (deltaMs > 0) {
        // --- Calculate Wind Speed ---
        // Save current pulses and reset counter via noInterrupts to prevent race condition
        noInterrupts();
        unsigned long pulses = windPulses;
        windPulses = 0;
        interrupts();

        // 2 ticks = 1 rotation (based on python code)
        float rotation_hz = (pulses / 2.0) / (deltaMs / 1000.0);
        float circumference = WIND_CM_RADIUS * 2.0 * PI;
        data.speed = rotation_hz * circumference * WIND_FACTOR;
        
        lastWindSampleTime = now;
        
        // --- Calculate Wind Direction ---
        // Target mV array from Python script (0.9V = 900mV, etc.)
        const float ADC_TO_MV[] = {900.0, 2000.0, 3000.0, 2800.0, 2500.0, 1500.0, 300.0, 600.0};
        
        // Read multiple times to stabilize like python while True loop
        int closest_index = -1;
        float closest_value = 99999.0;
        float value = (float)analogReadMilliVolts(WDIR_PIN);

        for (int i = 0; i < 8; i++) {
            float distance = abs(ADC_TO_MV[i] - value);
            if (distance < closest_value) {
                closest_value = distance;
                closest_index = i;
            }
        }
        
        if (closest_index != -1) {
            data.direction = closest_index * 45.0;
        }

        data.valid = true;
    }
#endif
    return data.valid;
}

bool Sensor::getRainData(RainData &data) {
    data.valid = false;
#if defined(RAIN_PIN)

    unsigned long now = millis();
    
    // Safely extract rain components
    noInterrupts();
    unsigned long daily = dailyRainTips;
    unsigned long t1 = beforeLastRainTipTime;
    unsigned long t2 = lastRainTipTime;
    interrupts();

    // The rate is mm per hour based on the time between the last two bucket tips.
    // If it hasn't rained (tipped) for > 5 minutes (300,000 ms), we consider rate as 0.
    if (t2 == 0 || (now - t2) > 300000UL) {
        data.rate = 0.0;
    } else if (t1 > 0 && t2 > t1) {
        unsigned long tipDeltaMs = t2 - t1;
        data.rate = (3600000.0 / tipDeltaMs) * RAIN_MM_PER_TICK;
    } else {
        data.rate = 0.0;
    }

    data.daily = daily * RAIN_MM_PER_TICK;
    data.valid = true;
    lastRainSampleTime = now;
    
#endif
    return data.valid;
}
