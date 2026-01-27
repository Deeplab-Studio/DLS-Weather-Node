#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BME680.h>
#include "DLSWeather.h"

// --- KONFIGURASYON ---
const int LED_PIN = 2; // Çoğu ESP32'de dahili LED GPIO 2'dedir
Preferences preferences;
String _ssid = "WIFI_SSID_GIRIN";
String _pass = "WIFI_SIFRE_GIRIN";
String _apiKey = "API_KEY";
String _stationId = "STATION_ID";
float _lat = 0.0;
float _lon = 0.0;

// --- NESNELER ---
DLSWeather* dls;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

// Sensor Nesneleri
Adafruit_BMP280 bmp;
Adafruit_BME280 bme;
Adafruit_BME680 bme680;

enum SensorType { NONE, BMP280, BME280, BME680 };
SensorType foundSensor = NONE;

unsigned long lastSendTime = 0;
int lastSentMinute = -1;

void checkSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.startsWith("ssid=")) {
            String val = cmd.substring(5);
            preferences.putString("ssid", val);
            _ssid = val;
            Serial.println("SSID Kaydedildi: " + val);
        } else if (cmd.startsWith("pass=")) {
            String val = cmd.substring(5);
            preferences.putString("pass", val);
            _pass = val;
            Serial.println("Sifre Kaydedildi.");
        } else if (cmd.startsWith("api=")) {
            String val = cmd.substring(4);
            preferences.putString("api", val);
            _apiKey = val;
            Serial.println("API Key Kaydedildi.");
        } else if (cmd.startsWith("station=")) {
            String val = cmd.substring(8);
            preferences.putString("station", val);
            _stationId = val;
            Serial.println("Station ID Kaydedildi.");
        } else if (cmd.startsWith("lat=")) {
            String valStr = cmd.substring(4);
            valStr.replace(',', '.');
            float val = valStr.toFloat();
            preferences.putFloat("lat", val);
            _lat = val;
            Serial.print("Lat Kaydedildi: ");
            Serial.println(val, 6);
        } else if (cmd.startsWith("lon=")) {
            String valStr = cmd.substring(4);
            valStr.replace(',', '.');
            float val = valStr.toFloat();
            preferences.putFloat("lon", val);
            _lon = val;
            Serial.print("Lon Kaydedildi: ");
            Serial.println(val, 6);
        } else if (cmd.equalsIgnoreCase("restart")) {
            Serial.println("Yeniden baslatiliyor...");
            delay(1000);
            ESP.restart();
        } else if (cmd.equalsIgnoreCase("info")) {
            Serial.println("--- Mevcut Ayarlar ---");
            Serial.println("SSID: " + _ssid);
            Serial.println("Station: " + _stationId);
            Serial.println("Lat: " + String(_lat, 6));
            Serial.println("Lon: " + String(_lon, 6));
        }
    }
}

void loadPreferences() {
    preferences.begin("dls-config", false);
    _ssid = preferences.getString("ssid", _ssid);
    _pass = preferences.getString("pass", _pass);
    _apiKey = preferences.getString("api", _apiKey);
    _stationId = preferences.getString("station", _stationId);
    _lat = preferences.getFloat("lat", _lat);
    _lon = preferences.getFloat("lon", _lon);
}

void detectSensor() {
    Serial.println("\n--- Coklu Sensor Tarama Baslatildi ---");
    Wire.begin(21, 22);

    if (bme680.begin(0x76)) {
        foundSensor = BME680;
        Serial.println("SENSOR: BME680 Tespit Edildi!");
        bme680.setTemperatureOversampling(BME680_OS_8X);
        bme680.setHumidityOversampling(BME680_OS_2X);
        bme680.setPressureOversampling(BME680_OS_4X);
        bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme680.setGasHeater(320, 150);
    } else if (bme.begin(0x76)) {
        foundSensor = BME280;
        Serial.println("SENSOR: BME280 Tespit Edildi!");
    } else if (bmp.begin(0x76)) {
        foundSensor = BMP280;
        Serial.println("SENSOR: BMP280 Tespit Edildi!");
    } else {
        Serial.println("HATA: Hicbir sensor bulunamadi!");
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // 1. Kayitli Ayarlari Yukle
    loadPreferences();

    Serial.println("\n--- Yuklu Ayarlar ---");
    Serial.println("SSID: " + _ssid);
    Serial.println("Station ID: " + _stationId);
    Serial.println("---------------------");

    // 2. Ayar Kontrolu
    if (_ssid == "WIFI_SSID_GIRIN" || _ssid.isEmpty()) {
        Serial.println("\n!!! AYARLAR EKSIK !!!");
        Serial.println("Lutfen Serial/Docs uzerinden ayarlari girin.");
        while (true) {
            checkSerialCommands();
            delay(10);
        }
    }

    // 3. Wi-Fi Baglantisi
    Serial.print("Wi-Fi Baglaniyor...");
    WiFi.begin(_ssid.c_str(), _pass.c_str());

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
        delay(250);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(LED_PIN, HIGH); // Baglaninca sabit yansin
        Serial.println("\nWi-Fi Baglandi!");
    } else {
        digitalWrite(LED_PIN, LOW); // Baglanamazsa sönsün
        Serial.println("\nWi-Fi Basarisiz. Loop'ta tekrar denenecek.");
    }

    // 4. NTP & Sensor & Library
    timeClient.begin();
    detectSensor();
    dls = new DLSWeather(_stationId, _apiKey, _lat, _lon);
    dls->begin();
}

void loop() {
    // Wi-Fi Yonetimi
    if (WiFi.status() != WL_CONNECTED) {
        digitalWrite(LED_PIN, LOW); // Baglanti koparsa LED sönsün
        static unsigned long lastReconnect = 0;
        if (millis() - lastReconnect > 10000) {
            WiFi.disconnect();
            WiFi.reconnect();
            lastReconnect = millis();
        }
        checkSerialCommands();
        return;
    } else {
        digitalWrite(LED_PIN, HIGH); // Bagli oldugu surece yansin
    }

    // Serial Komutlarini Dinle
    checkSerialCommands();

    timeClient.update();
    int currentMinute = timeClient.getMinutes();

    // Veri Gonderimi (0. ve 30. Dakikalar)
    if ((currentMinute == 0 || currentMinute == 30) && currentMinute != lastSentMinute) {
        Serial.println("\n--- Veriler Okunuyor ve Gonderiliyor ---");

        switch (foundSensor) {
            case BMP280:
                dls->temperature(bmp.readTemperature());
                dls->pressure(bmp.readPressure() / 100.0F);
                break;
            case BME280:
                dls->temperature(bme.readTemperature());
                dls->humidity(bme.readHumidity());
                dls->pressure(bme.readPressure() / 100.0F);
                break;
            case BME680:
                if (bme680.performReading()) {
                    dls->temperature(bme680.temperature);
                    dls->humidity(bme680.humidity);
                    dls->pressure(bme680.pressure / 100.0F);
                    dls->airQuality(bme680.gas_resistance / 1000.0);
                }
                break;
            default: break;
        }

        if (dls->send(timeClient.getEpochTime())) {
            Serial.println("Basariyla gonderildi.");
            lastSentMinute = currentMinute;
        } else {
            Serial.println("Gonderme hatasi!");
        }
    }

    delay(1000);
}
