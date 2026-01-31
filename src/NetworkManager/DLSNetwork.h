#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESPmDNS.h>

class DLSNetwork {
public:
    DLSNetwork();
    void begin(String ssid, String pass, int ledPin = -1);
    void update();
    void startMDNS(const char* hostname);
    
    // Status
    bool isConnected();
    
    // Time
    unsigned long getEpochTime();
    int getMinutes();
    int getSeconds();

private:
    WiFiUDP _ntpUDP;
    NTPClient* _timeClient;
    
    String _ssid;
    String _pass;
    int _ledPin;

    unsigned long _lastReconnectAttempt;
};
