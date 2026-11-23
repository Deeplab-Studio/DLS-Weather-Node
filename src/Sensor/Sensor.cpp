#include "Sensor.h"
#include <variant.h> // Pin tanimlari

// --- Kalibrasyon sabitleri ---
#define RAIN_MM_PER_TICK 0.2794f // kepce basina yagis (mm)
#define WIND_CM_RADIUS 7.0f
#define WIND_FACTOR 0.0218f
#define WIND_MIN_WINDOW_MS 2000UL // daha kisa pencerede hiz hesabi gurultulu

// --- Kesme paylasimli durum ---
static portMUX_TYPE s_isrMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t s_windPulses = 0;
static volatile uint32_t s_lastWindPulseMs = 0;
static volatile uint32_t s_rainTips = 0;
static volatile uint32_t s_lastRainTipMs = 0;
static volatile uint32_t s_prevRainTipMs = 0;

static uint32_t s_lastWindSampleMs = 0;
static WindData s_lastWind = {}; // kisa pencerede son gecerli deger

#if defined(WSPEED_PIN)
void IRAM_ATTR windSpeedISR() {
    uint32_t now = millis();
    if ((uint32_t)(now - s_lastWindPulseMs) > 15) { // reed switch debounce
        s_windPulses++;
        s_lastWindPulseMs = now;
    }
}
#endif

#if defined(RAIN_PIN)
void IRAM_ATTR rainISR() {
    uint32_t now = millis();
    if ((uint32_t)(now - s_lastRainTipMs) > 100) { // 100 ms debounce
        portENTER_CRITICAL_ISR(&s_isrMux);
        s_rainTips++;
        s_prevRainTipMs = s_lastRainTipMs;
        s_lastRainTipMs = now;
        portEXIT_CRITICAL_ISR(&s_isrMux);
    }
}
#endif

Sensor::Sensor() {}

bool Sensor::hasWindRain() {
#if defined(WDIR_PIN) && defined(WSPEED_PIN) && defined(RAIN_PIN)
    return true;
#else
    return false;
#endif
}

const char *Sensor::getAirSensorName() const {
    switch (_foundAirSensor) {
        case AIR_BME680: return "BME680";
        case AIR_BME280: return "BME280";
        case AIR_BMP280: return "BMP280";
        case AIR_SHTC3: return "SHTC3";
        case AIR_SHT3X: return "SHT3x";
        default: return "yok";
    }
}

const char *Sensor::getLightSensorName() const {
    return _foundLightSensor == LIGHT_VEML6075 ? "VEML6075" : "yok";
}

void Sensor::begin(TwoWire *wire) {
    _i2c = wire ? wire : &Wire;
    Serial.println("\n[Sensor] Taramasi baslatiliyor...");

    // --- HAVA SENSORLERI ---
    // Not: Adafruit suruculerinin bir kismi I2C bus parametresi almiyor
    // (varsayilan &Wire kullanilir); Wire setup() icinde baslatilir.
    if (_bme680.begin(0x76) || _bme680.begin(0x77)) {
        _foundAirSensor = AIR_BME680;
        Serial.println("[Sensor] BME680 tespit edildi.");
        _bme680.setTemperatureOversampling(BME680_OS_8X);
        _bme680.setHumidityOversampling(BME680_OS_2X);
        _bme680.setPressureOversampling(BME680_OS_4X);
        _bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
        _bme680.setGasHeater(320, 150);
    } else if (_sht31.begin(0x44) || _sht31.begin(0x45)) {
        _foundAirSensor = AIR_SHT3X;
        Serial.println("[Sensor] SHT3x (SHT30/31/35) tespit edildi.");
    } else if (_shtc3.begin(_i2c)) {
        _foundAirSensor = AIR_SHTC3;
        Serial.println("[Sensor] SHTC3 tespit edildi.");
    } else if (_bme280.begin(0x76, _i2c)) {
        _foundAirSensor = AIR_BME280;
        Serial.println("[Sensor] BME280 tespit edildi.");
    } else if (_bmp280.begin(0x76)) {
        _foundAirSensor = AIR_BMP280;
        Serial.println("[Sensor] BMP280 tespit edildi.");
    } else {
        Serial.println("[Sensor] Hicbir hava sensoru bulunamadi.");
    }

    // --- ISIK / UV ---
    if (_veml6075.begin(VEML6075_100MS, false, false, _i2c)) {
        _foundLightSensor = LIGHT_VEML6075;
        Serial.println("[Sensor] VEML6075 (UV) tespit edildi.");
    } else {
        Serial.println("[Sensor] UV sensoru bulunamadi.");
    }

    // --- RUZGAR / YAGMUR ---
#if defined(WDIR_PIN) && defined(WSPEED_PIN) && defined(RAIN_PIN)
    pinMode(WDIR_PIN, INPUT);
    pinMode(WSPEED_PIN, INPUT_PULLUP);
    pinMode(RAIN_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(WSPEED_PIN), windSpeedISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(RAIN_PIN), rainISR, FALLING);
    s_lastWindSampleMs = millis();
    Serial.println("[Sensor] Ruzgar/ygmur kesmeleri aktif.");
#else
    Serial.println("[Sensor] Bu kartta ruzgar/ygmur pinleri tanimli degil.");
#endif
}

bool Sensor::getAirData(AirData &data) {
    data = AirData(); // NaN'lara sifirla

    switch (_foundAirSensor) {
        case AIR_BME680: {
            if (!_bme680.performReading()) return false;
            data.temperature = _bme680.temperature;
            data.humidity = _bme680.humidity;
            data.pressure = _bme680.pressure / 100.0f;
            data.gasResistance = _bme680.gas_resistance / 1000.0f; // kOhm
            break;
        }
        case AIR_SHT3X: {
            data.temperature = _sht31.readTemperature();
            data.humidity = _sht31.readHumidity();
            break;
        }
        case AIR_SHTC3: {
            sensors_event_t h, t;
            if (!_shtc3.getEvent(&h, &t)) return false;
            data.temperature = t.temperature;
            data.humidity = h.relative_humidity;
            break;
        }
        case AIR_BME280: {
            data.temperature = _bme280.readTemperature();
            data.humidity = _bme280.readHumidity();
            data.pressure = _bme280.readPressure() / 100.0f;
            break;
        }
        case AIR_BMP280: {
            data.temperature = _bmp280.readTemperature();
            data.pressure = _bmp280.readPressure() / 100.0f;
            break;
        }
        case AIR_NONE:
        default:
            return false;
    }

    // Olcum yapmayan alanlar NaN kalir; valid yalnizca gecerli sayilarla true.
    if (!isfinite(data.temperature)) data = AirData();
    else {
        if (!isfinite(data.humidity)) data.humidity = NAN;
        if (!isfinite(data.pressure)) data.pressure = NAN;
        if (!isfinite(data.gasResistance) || data.gasResistance <= 0) data.gasResistance = NAN;
        data.valid = true;
    }
    return data.valid;
}

bool Sensor::getLightData(LightData &data) {
    data = LightData();
    if (_foundLightSensor != LIGHT_VEML6075) return false;

    data.uva = _veml6075.readUVA();
    data.uvb = _veml6075.readUVB();
    data.uvIndex = _veml6075.readUVI();
    if (!isfinite(data.uvIndex) || data.uvIndex < 0) {
        data = LightData();
        return false;
    }
    data.valid = true;
    return true;
}

bool Sensor::getWindData(WindData &data) {
    data = WindData();
#if defined(WDIR_PIN) && defined(WSPEED_PIN)
    uint32_t now = millis();
    uint32_t delta = now - s_lastWindSampleMs;

    // Cok kisa pencerede hiz hesabi anlamsiz: son gecerli degeri dondur.
    if (delta < WIND_MIN_WINDOW_MS) {
        data = s_lastWind;
        return data.valid;
    }

    uint32_t pulses;
    portENTER_CRITICAL(&s_isrMux);
    pulses = s_windPulses;
    s_windPulses = 0;
    portEXIT_CRITICAL(&s_isrMux);
    s_lastWindSampleMs = now;

    // 2 darbe = 1 tur
    float rotationHz = (pulses / 2.0f) / (delta / 1000.0f);
    float circumference = WIND_CM_RADIUS * 2.0f * PI;
    data.speed = rotationHz * circumference * WIND_FACTOR;

    // --- Yon: 8 kademeli direnc agi (mV esikleri) ---
    static const float ADC_TO_MV[8] = {900.0f, 2000.0f, 3000.0f, 2800.0f,
                                       2500.0f, 1500.0f, 300.0f, 600.0f};
    uint32_t mvSum = 0;
    uint32_t samples = 4;
    for (uint32_t i = 0; i < samples; i++) mvSum += analogReadMilliVolts(WDIR_PIN);
    float value = (float)(mvSum / samples);

    int closest = 0;
    float closestDist = 1e9f;
    for (int i = 0; i < 8; i++) {
        float d = fabsf(ADC_TO_MV[i] - value);
        if (d < closestDist) {
            closestDist = d;
            closest = i;
        }
    }
    data.direction = closest * 45.0f;

    data.valid = isfinite(data.speed) && isfinite(data.direction);
    if (data.valid) s_lastWind = data;
#endif
    return data.valid;
}

bool Sensor::getRainData(RainData &data) {
    data = RainData();
#if defined(RAIN_PIN)
    uint32_t tips, t1, t2;
    portENTER_CRITICAL(&s_isrMux);
    tips = s_rainTips;
    t1 = s_prevRainTipMs;
    t2 = s_lastRainTipMs;
    portEXIT_CRITICAL(&s_isrMux);

    uint32_t now = millis();
    // Son 5 dakikada kepce atmadiysa yagis hizi 0 kabul edilir.
    if (t2 == 0 || (uint32_t)(now - t2) > 300000UL) {
        data.rate = 0.0f;
    } else if (t1 != 0 && t2 != t1) {
        uint32_t deltaMs = t2 - t1; // tasma guvenli
        data.rate = (3600000.0f / (float)deltaMs) * RAIN_MM_PER_TICK;
    } else {
        data.rate = 0.0f;
    }

    data.daily = _rainDailyBase + (float)tips * RAIN_MM_PER_TICK;
    data.valid = isfinite(data.rate) && isfinite(data.daily);
#endif
    return data.valid;
}

void Sensor::restoreRainDaily(float mm) {
    if (isfinite(mm) && mm >= 0) _rainDailyBase = mm;
}

float Sensor::beginRainDay(int epochDay) {
#if defined(RAIN_PIN)
    if (epochDay <= 0) return _rainDailyBase;
    if (_rainDay < 0) { // ilk cagri: gunu kaydet
        _rainDay = epochDay;
        return _rainDailyBase;
    }
    portENTER_CRITICAL(&s_isrMux);
    uint32_t tips = s_rainTips;
    portEXIT_CRITICAL(&s_isrMux);

    if (epochDay != _rainDay) { // gece yarisi gecildi -> sayac sifirlanir
        portENTER_CRITICAL(&s_isrMux);
        s_rainTips = 0;
        portEXIT_CRITICAL(&s_isrMux);
        _rainDailyBase = 0.0f;
        _rainDay = epochDay;
        return 0.0f;
    }
    return _rainDailyBase + (float)tips * RAIN_MM_PER_TICK;
#else
    (void)epochDay;
    return 0.0f;
#endif
}
