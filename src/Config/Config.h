#pragma once

#include <Arduino.h>
#include <Preferences.h>

class Config {
public:
    Config();
    void begin();
    void checkSerialCommands();

    // Getters
    String getSSID() const { return _ssid; }
    String getPass() const { return _pass; }
    String getAPIKey() const { return _apiKey; }
    String getStationID() const { return _stationId; }
    float getLat() const { return _lat; }
    float getLon() const { return _lon; }
    int getInterval() const { return _intervalMin; }
    bool isDeepSleepEnabled() const { return _isDeepSleepEnabled; }

private:
    Preferences _prefs;
    
    // Vars
    String _ssid;
    String _pass;
    String _apiKey;
    String _stationId;
    float _lat;
    float _lon;
    int _intervalMin;
    bool _isDeepSleepEnabled;

    void load();
    void info();
};
