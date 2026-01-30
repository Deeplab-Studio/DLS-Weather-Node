#include "Config.h"

Config::Config() {
    _ssid = "WIFI_SSID_GIRIN";
    _pass = "WIFI_SIFRE_GIRIN";
    _apiKey = "API_KEY";
    _stationId = "STATION_ID";
    _lat = 0.0;
    _lon = 0.0;
    _intervalMin = 30; // Default 30 mins
}

void Config::begin() {
    _prefs.begin("dls-config", false);
    load();
}

void Config::load() {
    _ssid = _prefs.getString("ssid", _ssid);
    _pass = _prefs.getString("pass", _pass);
    _apiKey = _prefs.getString("api", _apiKey);
    _stationId = _prefs.getString("station", _stationId);
    _lat = _prefs.getFloat("lat", _lat);
    _lon = _prefs.getFloat("lon", _lon);
    _intervalMin = _prefs.getInt("interval", _intervalMin);
}

void Config::checkSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.startsWith("ssid=")) {
            String val = cmd.substring(5);
            _prefs.putString("ssid", val);
            _ssid = val;
            Serial.println("SSID Kaydedildi: " + val);
        } else if (cmd.startsWith("pass=")) {
            String val = cmd.substring(5);
            _prefs.putString("pass", val);
            _pass = val;
            Serial.println("Sifre Kaydedildi.");
        } else if (cmd.startsWith("api=")) {
            String val = cmd.substring(4);
            _prefs.putString("api", val);
            _apiKey = val;
            Serial.println("API Key Kaydedildi.");
        } else if (cmd.startsWith("station=")) {
            String val = cmd.substring(8);
            _prefs.putString("station", val);
            _stationId = val;
            Serial.println("Station ID Kaydedildi.");
        } else if (cmd.startsWith("lat=")) {
            String valStr = cmd.substring(4);
            valStr.replace(',', '.');
            float val = valStr.toFloat();
            _prefs.putFloat("lat", val);
            _lat = val;
            Serial.print("Lat Kaydedildi: "); Serial.println(val, 6);
        } else if (cmd.startsWith("lon=")) {
            String valStr = cmd.substring(4);
            valStr.replace(',', '.');
            float val = valStr.toFloat();
            _prefs.putFloat("lon", val);
            _lon = val;
            Serial.print("Lon Kaydedildi: "); Serial.println(val, 6);
        } else if (cmd.startsWith("interval=")) {
            int val = cmd.substring(9).toInt();
            if (val < 1) val = 1; 
            _prefs.putInt("interval", val);
            _intervalMin = val;
            Serial.print("Interval Guncellendi (dk): "); Serial.println(val);
        } else if (cmd.equalsIgnoreCase("restart")) {
            Serial.println("Yeniden baslatiliyor...");
            delay(1000);
            ESP.restart();
        } else if (cmd.equalsIgnoreCase("info")) {
            info();
        }
    }
}

void Config::info() {
    Serial.println("--- Mevcut Ayarlar ---");
    Serial.println("SSID: " + _ssid);
    Serial.println("Station: " + _stationId);
    Serial.println("Lat: " + String(_lat, 6));
    Serial.println("Lon: " + String(_lon, 6));
    Serial.println("Interval: " + String(_intervalMin) + " dk");
}
