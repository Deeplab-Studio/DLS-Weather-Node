#pragma once

#include <Arduino.h>
#include <Preferences.h>

class Config {
public:
    Config();
    void begin();
    void checkSerialCommands();

    // Save all settings at once
    void saveConfig(String ssid, String pass, String apiKey, String stationId, String alias, float lat, float lon, int interval, bool deepSleep, int txPower);

    // Getters
    // Getters
    String getSSID() const { return _ssid; }
    String getPass() const { return _pass; }
    String getAPIKey() const { return _apiKey; }
    String getStationID() const { return _stationId; }
    String getAlias() const { return _alias; } // NEW
    float getLat() const { return _lat; }
    float getLon() const { return _lon; }
    int getInterval() const { return _intervalMin; }
    bool isDeepSleepEnabled() const { return _isDeepSleepEnabled; }
    int getTxPower() const { return _txPower; }

private:
    Preferences _prefs;
    
    // Vars
    String _ssid;
    String _pass;
    String _apiKey;
    String _stationId;
    String _alias; // NEW
    float _lat;
    float _lon;
    int _intervalMin;
    bool _isDeepSleepEnabled;
    int _txPower; // Stores wifi_power_t enum value as int

    void load();
    void info();
};
