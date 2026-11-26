#include <Wire.h>
#include <ArduinoJson.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/gpio.h>

#include "DLSWeather.h"
#include "variant.h"
#include "version.h"
#include "Sensor/Sensor.h"
#include "NetworkManager/DLSNetwork.h"
#include "Display/Display.h"
#include "Config/Config.h"
#include "WebUI/WebUI.h"

// --- NESNELER ---
Config config;
Sensor sensorManager;
DLSNetwork network;
Display display;
WebUI webui;

// --- SON OKUNAN VERILER (ekran + web icin) ---
AirData latestAir;
LightData latestLight;
WindData latestWind;
RainData latestRain;
bool battValid = false;
float battVoltage = NAN;
int battPct = -1;

// --- ZAMANLAMA DURUMU ---
static const uint32_t CONFIG_WINDOW_MS = 5UL * 60UL * 1000UL; // ilk acilista ayar penceresi
static const uint32_t RETRY_DELAY_MS = 60UL * 1000UL;
static const uint8_t MAX_RETRIES = 3;

bool isFromSleep = false;
bool configWindowActive = false;
bool firstSendDone = false;
unsigned long lastSendEpoch = 0;
unsigned long lastAttemptMs = 0;
uint8_t retryCount = 0;
unsigned long bootMs = 0;
int rainDay = 0;

// ---------------------------------------------------------------------------
// Yardimcilar
// ---------------------------------------------------------------------------
#ifdef ADC_PIN
static float readBatteryVoltage() {
    uint32_t sum = 0;
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++) sum += analogReadMilliVolts(ADC_PIN);
    float mV = (float)sum / (float)ADC_SAMPLE_COUNT;
    return (mV / 1000.0f) * ADC_MULTIPLIER;
}

static int batteryPercent(float v) {
    if (!isfinite(v)) return -1;
    float p = (v - 3.0f) / (4.2f - 3.0f) * 100.0f;
    if (p < 0.0f) p = 0.0f;
    if (p > 100.0f) p = 100.0f;
    return (int)(p + 0.5f);
}
#endif

static void readAllSensors() {
    sensorManager.getAirData(latestAir);
    sensorManager.getLightData(latestLight);
    sensorManager.getWindData(latestWind);
    sensorManager.getRainData(latestRain);

#ifdef ADC_PIN
    battVoltage = readBatteryVoltage();
    battPct = batteryPercent(battVoltage);
    battValid = isfinite(battVoltage) && battVoltage > 2.0f;
#endif

    // Gunluk yagis: gece yarisinda sifirlanir, NVS'e yazilir (uyku sonrasi korunur)
    if (network.timeSynced()) {
        int day = network.getEpochDay();
        if (rainDay != day) {
            if (rainDay == 0 && config.getStoredRainDay() == day) {
                sensorManager.restoreRainDaily(config.getStoredRainDaily());
            }
            float daily = sensorManager.beginRainDay(day);
            rainDay = day;
            config.setRainDaily(daily, day);
            latestRain.daily = daily;
        }
    }

    display.setAirData(latestAir.temperature, latestAir.humidity, latestAir.pressure,
                       latestAir.gasResistance);
    display.setLightData(latestLight.uvIndex);
    display.setWindData(latestWind.speed, latestWind.direction);
    display.setRainData(latestRain.rate, latestRain.daily);
    if (battValid) display.setBatteryData(battVoltage, battPct);

    webui.updateSnapshot(latestAir, latestLight, latestWind, latestRain, battValid, battVoltage,
                         battPct);
}

static void logSensorData() {
#if DEBUG_MODE
    Serial.println("[Sensor]");
    if (latestAir.valid) {
        Serial.printf("  Temp %.1f C | Hum %.1f %% | Pres %.1f hPa\n", latestAir.temperature,
                      latestAir.humidity, latestAir.pressure);
        if (isfinite(latestAir.gasResistance)) Serial.printf("  Gas  %.1f kOhm\n", latestAir.gasResistance);
    }
    if (latestLight.valid) Serial.printf("  UV   %.2f\n", latestLight.uvIndex);
    if (latestWind.valid) Serial.printf("  Wind %.2f m/s @ %.0f deg\n", latestWind.speed, latestWind.direction);
    if (latestRain.valid) Serial.printf("  Rain %.2f mm/h | %.2f mm today\n", latestRain.rate, latestRain.daily);
    if (battValid) Serial.printf("  Bat  %.2f V (%d%%)\n", battVoltage, battPct);
#endif
}

// ---------------------------------------------------------------------------
// Gonderim
// ---------------------------------------------------------------------------
static bool sendWeather() {
    unsigned long epoch = network.getEpochTime();
    display.setStatus("Gonderiliyor");
    display.update();

    // Her gonderimde yeni nesne: onceki okumadan kalan deger sizmaz.
    DLSWeather dls(config.getStationID(), config.getAPIKey(), config.getLat(), config.getLon());

    if (latestAir.valid) {
        if (isfinite(latestAir.temperature)) dls.temperature(latestAir.temperature);
        if (isfinite(latestAir.humidity)) dls.humidity(latestAir.humidity);
        if (isfinite(latestAir.pressure)) dls.pressure(latestAir.pressure);
        if (isfinite(latestAir.gasResistance) && latestAir.gasResistance > 0)
            dls.airQuality((int)(latestAir.gasResistance + 0.5f)); // kOhm
    }
    if (latestLight.valid && isfinite(latestLight.uvIndex)) dls.uvIndex(latestLight.uvIndex);
    if (latestWind.valid) {
        if (isfinite(latestWind.speed)) dls.windSpeed(latestWind.speed);
        if (isfinite(latestWind.direction)) dls.windDirection(latestWind.direction);
    }
    if (latestRain.valid) {
        if (isfinite(latestRain.rate)) dls.rainRate(latestRain.rate);
        if (isfinite(latestRain.daily)) dls.rainDaily(latestRain.daily);
    }
    if (battValid) {
        dls.battery(battPct);
        dls.voltage(battVoltage);
    }

    bool ok = dls.send(epoch);
    int code = dls.getLastCode();

    if (ok) {
        Serial.printf("[Send] OK (HTTP %d) epoch=%lu\n", code, epoch);
        display.setStatus("Gonderildi");
        webui.setSendStatus("OK " + String(code));
        lastSendEpoch = epoch;
        retryCount = 0;
        firstSendDone = true;
        return true;
    }

    String errText = !network.isConnected() ? String("WiFi yok") : (code > 0 ? "HTTP " + String(code) : "Baglanti hatasi");
    Serial.println("[Send] HATA: " + errText);
    display.setStatus(errText, true);
    webui.setSendStatus(errText);
    return false;
}

static void enterDeepSleep() {
    int interval = config.getInterval();
    if (interval <= 0) interval = 30;
    long sleepSeconds = (long)interval * 60L;

    Serial.printf("[Sleep] %ld saniye derin uykuya giriliyor.\n", sleepSeconds);
    display.setStatus("Uyuyor");
    display.update();
    delay(300);
    display.off();

    // Sensor guc MOSFET'ini uyku boyunca kapali tut
    digitalWrite(SENSOR_PWR_PIN, LOW);
    gpio_hold_en((gpio_num_t)SENSOR_PWR_PIN);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_timer_wakeup((uint64_t)sleepSeconds * 1000000ULL);
    Serial.flush();
    esp_deep_sleep_start();
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------
void setup() {
    pinMode(SENSOR_PWR_PIN, OUTPUT);
    digitalWrite(SENSOR_PWR_PIN, HIGH);

    Serial.begin(115200);
    bootMs = millis();
    isFromSleep = (esp_reset_reason() == ESP_RST_DEEPSLEEP);

    // Uykudan uyanisda sensorlerin oturmasi icin kisa bekleme yeterli
    delay(isFromSleep ? 500 : 1200);

    config.begin();

    Serial.println("\n=============================");
    Serial.println("  DLS Weather Node " DLS_FW_VERSION);
    Serial.println("=============================");
    config.checkSerialCommands(); // boot sirasinda komut yakalama sansi

    Serial.println("SSID: " + config.getSSID());
    Serial.println("Istasyon: " + config.getStationID());
    if (config.getAlias().length()) Serial.println("Ad: " + config.getAlias());
    Serial.println("Aralik: " + String(config.getInterval()) + " dk");
    Serial.println("Web PIN: " + config.getWebPin());

    Wire.begin(I2C_SDA, I2C_SCL);

    // Yuksek empedansli bolucu / ruzgar yonu icin ADC zayiflatma
#ifdef ADC_PIN
    analogSetPinAttenuation(ADC_PIN, ADC_11db);
#endif
#if defined(WDIR_PIN)
    analogSetPinAttenuation(WDIR_PIN, ADC_11db);
#endif

    display.begin(&Wire);
    sensorManager.begin(&Wire);

    display.setWebPin(config.getWebPin());
    display.setAvailableSensors(sensorManager.hasAir(), sensorManager.hasLight(),
                                Sensor::hasWindRain());
    display.printStartup(config.getSSID(), config.getAlias(), config.getWebPin());

    if (!config.isConfigured()) {
        display.showMessage("Ayar eksik! Seri port: SET_CONFIG");
        Serial.println("\n!!! AYARLAR EKSIK !!!");
        Serial.println("Seri porttan 'SET_CONFIG {json}' veya web installer kullanin.");
        while (true) {
            config.checkSerialCommands();
            delay(20);
        }
    }

    network.setHostname(config.getHostname());
    network.setTxPower((wifi_power_t)config.getTxPower());
    network.begin(config.getSSID(), config.getPass(), LED_PIN);

    sensorManager.restoreRainDaily(0.0f);

    webui.setSensorsReport(String(sensorManager.getAirSensorName()) + " / UV:" +
                          String(sensorManager.getLightSensorName()) +
                          (Sensor::hasWindRain() ? " / ruzgar+yagmur" : ""));
    webui.begin(&config, &network);

    if (network.waitConnected(20000)) {
        Serial.printf("[Net] Web UI: http://%s\n", WiFi.localIP().toString().c_str());
        // Gonderim icin gecerli zaman gerekli: en fazla 15 sn bekle
        uint32_t waitStart = millis();
        while (!network.timeSynced() && millis() - waitStart < 15000) {
            network.update();
            webui.handle();
            delay(50);
        }
        if (!network.timeSynced()) Serial.println("[NTP] Zaman alinamadi, sonra denenecek.");
    } else {
        Serial.println("[Net] WiFi baglantisi kurulamadi (5 dk sonra yeniden baslatilir).");
    }

    // Derin uyku modunda ilk acilista ayar penceresi birak
    configWindowActive = config.isDeepSleepEnabled() && !isFromSleep;
    lastAttemptMs = millis();
    Serial.println(configWindowActive ? "[Mode] 5 dk ayar penceresi acik."
                                      : "[Mode] Olcum dongusu basladi.");
}

void loop() {
    network.update();
    webui.handle();
    config.checkSerialCommands();

    display.setNetworkInfo(network.isConnected() ? WiFi.localIP().toString() : String("0.0.0.0"),
                           config.getSSID(), network.isConnected() ? "Online" : "Offline",
                           network.isConnected());
    display.update();

    // --- 1 sn'de bir sensor okuma (ekran + web + gonderim ayni veriyi kullanir) ---
    static unsigned long lastRead = 0;
    if (millis() - lastRead >= 1000) {
        lastRead = millis();
        readAllSensors();
        logSensorData();
    }

    uint32_t now = millis();
    int interval = config.getInterval();
    if (interval <= 0) interval = 30;

    // --- Gonderim zamani geldi mi? ---
    bool attempt = false;
    if (configWindowActive) {
        if (now - bootMs > CONFIG_WINDOW_MS) {
            configWindowActive = false;
            attempt = true;
            Serial.println("[Mode] Ayar penceresi kapandi, ilk gonderim.");
        }
    } else if (retryCount > 0 && retryCount <= MAX_RETRIES && now - lastAttemptMs > RETRY_DELAY_MS) {
        attempt = true;
    } else if (!firstSendDone && retryCount > MAX_RETRIES) {
        // Ilk gonderim 3 denemede de basarisiz olduysa 15 sn'de bir tekrarlamamak icin interval kadar bekle
        attempt = (now - lastAttemptMs) >= (unsigned long)interval * 60000UL;
        if (attempt) retryCount = 0;
    } else if (retryCount == 0) {
        if (!firstSendDone) {
            // Saat yoksa 5 dakika sonra yine de dene (aksi halde NTP engelliyse hic gonderilmez)
            attempt = network.isConnected() &&
                      (network.timeSynced() || (now - bootMs > 300000UL));
        } else if (network.timeSynced()) {
            attempt = (lastSendEpoch == 0) || (network.getEpochTime() >= lastSendEpoch + (unsigned long)interval * 60UL);
        } else {
            // Saat yoksa: son denemeden bu yana interval kadar gectiyse gonder
            attempt = (now - lastAttemptMs) >= (unsigned long)interval * 60000UL;
        }
    }

    if (attempt) {
        lastAttemptMs = now;
        readAllSensors(); // gonderimden hemen once taze okuma

        bool ok = false;
        if (network.isConnected()) {
            ok = sendWeather();
        } else {
            Serial.println("[Send] WiFi bagli degil.");
            display.setStatus("WiFi yok", true);
            webui.setSendStatus("WiFi yok");
        }

        if (ok) {
            if (config.isDeepSleepEnabled()) enterDeepSleep();
        } else {
            retryCount++;
            if (retryCount > MAX_RETRIES) {
                Serial.println("[Send] Deneme siniri asildi, sonraki aralik beklenecek.");
                if (config.isDeepSleepEnabled()) {
                    retryCount = 0;
                    enterDeepSleep();
                }
            }
        }
    }

    delay(10);
}
