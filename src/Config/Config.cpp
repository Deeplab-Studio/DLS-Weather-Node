#include "Config.h"
#include <ArduinoJson.h>

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
        
        // --- JSON BASED CONFIG ---
        if (cmd.equals("GET_CONFIG")) {
            JsonDocument doc;
            doc["ssid"] = _ssid;
            doc["pass"] = _pass;
            doc["api"] = _apiKey;
            doc["station"] = _stationId;
            doc["lat"] = _lat;
            doc["lon"] = _lon;
            doc["interval"] = _intervalMin;
            
            String response;
            serializeJson(doc, response);
            Serial.println(response);
        }
        else if (cmd.startsWith("SET_CONFIG ")) {
            String jsonStr = cmd.substring(11);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonStr);
            
            if (!error) {
                if (doc.containsKey("ssid")) _ssid = doc["ssid"].as<String>();
                if (doc.containsKey("pass")) _pass = doc["pass"].as<String>();
                if (doc.containsKey("api")) _apiKey = doc["api"].as<String>();
                if (doc.containsKey("station")) _stationId = doc["station"].as<String>();
                if (doc.containsKey("lat")) _lat = doc["lat"].as<float>();
                if (doc.containsKey("lon")) _lon = doc["lon"].as<float>();
                if (doc.containsKey("interval")) _intervalMin = doc["interval"].as<int>();
                
                // Save all
                _prefs.putString("ssid", _ssid);
                _prefs.putString("pass", _pass);
                _prefs.putString("api", _apiKey);
                _prefs.putString("station", _stationId);
                _prefs.putFloat("lat", _lat);
                _prefs.putFloat("lon", _lon);
                _prefs.putInt("interval", _intervalMin);
                
                Serial.println("CONFIG_SAVED");
                delay(500);
                ESP.restart();
            } else {
                Serial.println("JSON_ERROR");
            }
        }
        // --- LEGACY COMMANDS (Optional fallback) ---
        else if (cmd.startsWith("ssid=")) {
            // ... (keep legacy if desired, but user wants JSON now) ...
            // Let's keep basics for simple manual terminal usage
            String val = cmd.substring(5);
            _prefs.putString("ssid", val);
            _ssid = val;
            Serial.println("SSID Kaydedildi: " + val);
        } 
        // ... (Simplified legacy blocks or remove if strictly JSON preferred) ...
        // Keeping "restart" command for utility
        else if (cmd.equalsIgnoreCase("restart")) {
            Serial.println("Yeniden baslatiliyor...");
            delay(1000);
            ESP.restart();
        } 
        else if (cmd.equalsIgnoreCase("info")) {
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
