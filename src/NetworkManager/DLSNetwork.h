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
    bool startMDNS(const char *hostname);

    void setTxPower(wifi_power_t power);
    void onWiFiEvent(WiFiEvent_t event);

    // Durum
    bool isConnected();
    int getRSSI();

    // Zaman
    unsigned long getEpochTime();
    int getEpochDay();
    bool timeSynced();
    int getMinutes();
    int getSeconds();

    // Baglanti bekleme (setup icin, blocking ama sinirli)
    bool waitConnected(uint32_t timeoutMs);

    static bool isValidTxPower(int v);

private:
    WiFiUDP _ntpUDP;
    NTPClient *_timeClient = nullptr;

    String _ssid;
    String _pass;
    int _ledPin = -1;
    wifi_power_t _txPower = WIFI_POWER_8_5dBm;
    uint32_t _lastReconnectAttempt = 0;

    void setLed(bool on);
};
