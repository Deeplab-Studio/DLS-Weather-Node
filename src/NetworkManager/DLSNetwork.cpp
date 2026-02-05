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

    Serial.print("\n[WiFi] Connecting to: "); Serial.println(_ssid);
    
    // 1. Clean old state
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    // 2. Set mode and start
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false); // Disable power saving for max compatibility

    WiFi.begin(_ssid.c_str(), _pass.c_str());

    // 3. Wait for connection (Aggressive check)
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 30) { // Increased to 30 (15 sec)
        if (_ledPin != -1) {
            digitalWrite(_ledPin, !digitalRead(_ledPin)); // Toggle
        }
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        if (_ledPin != -1) digitalWrite(_ledPin, HIGH); 
        Serial.println("\n[WiFi] CONNECTED!");
        Serial.print("[WiFi] IP: "); Serial.println(WiFi.localIP());
        Serial.print("[WiFi] Mac: "); Serial.println(WiFi.macAddress());
        
        // Force time update immediately
        _timeClient->begin();
        _timeClient->forceUpdate();
    } else {
        if (_ledPin != -1) digitalWrite(_ledPin, LOW);
        Serial.println("\n[WiFi] FAILED to connect. Will retry in loop.");
        // Don't turn off WiFi here, let the loop handle retry
    }
}

void DLSNetwork::update() {
    // Wi-Fi Reconnect Logic
    if (WiFi.status() != WL_CONNECTED) {
        if (_ledPin != -1) digitalWrite(_ledPin, LOW);
        
        static unsigned long lastCheck = 0;
        // Check every 10 seconds if disconnected
        if (millis() - _lastReconnectAttempt > 10000) {
            _lastReconnectAttempt = millis();
            Serial.println("[WiFi] Lost connection. Reconnecting...");
            // Don't use disconnect(true) here as it might disrupt partial connection attempts
            // Just try begin again
            WiFi.reconnect(); 
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

int DLSNetwork::getSeconds() {
    return _timeClient->getSeconds();
}

void DLSNetwork::startMDNS(const char* hostname) {
    if (MDNS.begin(hostname)) {
        Serial.printf("[mDNS] Started: %s.local\n", hostname);
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("dls_weather", "udp", 12345); 
    } else {
        Serial.println("[mDNS] Failed to start!");
    }
}
