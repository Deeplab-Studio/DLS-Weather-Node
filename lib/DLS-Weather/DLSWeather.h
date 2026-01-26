#ifndef DLSWeather_h
#define DLSWeather_h

#include "Arduino.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BME680.h>

enum SensorType { NONE, BMP280, BME280, BME680 };

class DLSWeather {
public:
    DLSWeather();

    void begin(); // Başlangıç ayarları (WiFi, Sensör, NTP, Preferences)
    void handle(); // Loop içinde sürekli çalışacak fonksiyon

    void setEnvironment(float temp, float humidity, float pressure);
    void setWind(float speed, float direction);
    void setRain(float rate, float daily);

private:
    void loadPreferences();
    void detectSensor();
    void connectWiFi();
    void checkSerialCommands();
    bool send();

    // Ayarlar
    String _ssid;
    String _pass;
    String _apiKey;
    String _stationId;
    float _lat;
    float _lon;

    // Durum Değişkenleri
    float _temp;
    float _humidity;
    float _pressure;
    float _windSpeed;
    float _windDir;
    float _rainRate;
    float _rainDaily;

    // Nesneler
    Preferences _preferences;
    WiFiUDP _ntpUDP;
    NTPClient* _timeClient;

    // Sensörler
    Adafruit_BMP280 _bmp;
    Adafruit_BME280 _bme;
    Adafruit_BME680 _bme680;
    SensorType _foundSensor = NONE;

    // Zamanlama
    int _lastSentMinute = -1;
    bool _firstRun = true;
};

#endif
