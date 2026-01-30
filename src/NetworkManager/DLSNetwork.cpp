#include "DLSNetwork.h"

DLSNetwork::DLSNetwork() {
    _timeClient = new NTPClient(_ntpUDP, "pool.ntp.org", 0, 60000);
    _lastReconnectAttempt = 0;
    _ledPin = -1;
}

void DLSNetwork::begin(String ssid, String pass, int ledPin) {
    _ssid = ssid;
    _pass = pass;
    _ledPin = ledPin;

    if (_ledPin != -1) {
        pinMode(_ledPin, OUTPUT);
        digitalWrite(_ledPin, LOW);
    }

    Serial.print("Wi-Fi Baglaniyor...");
    WiFi.begin(_ssid.c_str(), _pass.c_str());

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        if (_ledPin != -1) {
            digitalWrite(_ledPin, HIGH);
            delay(250);
            digitalWrite(_ledPin, LOW);
            delay(250);
        } else {
            delay(500);
        }
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (_ledPin != -1) digitalWrite(_ledPin, HIGH); 
        Serial.println("\nWi-Fi Baglandi!");
        _timeClient->begin();
    } else {
        if (_ledPin != -1) digitalWrite(_ledPin, LOW);
        Serial.println("\nWi-Fi Basarisiz. Loop'ta tekrar denenecek.");
    }
}

void DLSNetwork::update() {
    // Wi-Fi Reconnect Logic
    if (WiFi.status() != WL_CONNECTED) {
        if (_ledPin != -1) digitalWrite(_ledPin, LOW);
        
        if (millis() - _lastReconnectAttempt > 10000) {
            Serial.println("Wi-Fi Kopuk. Tekrar baglaniyor...");
            WiFi.disconnect();
            WiFi.reconnect();
            _lastReconnectAttempt = millis();
        }
    } else {
         if (_ledPin != -1) digitalWrite(_ledPin, HIGH);
         _timeClient->update();
    }
}

bool DLSNetwork::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

unsigned long DLSNetwork::getEpochTime() {
    return _timeClient->getEpochTime();
}

int DLSNetwork::getMinutes() {
    return _timeClient->getMinutes();
}
