#include "DLSNetwork.h"

DLSNetwork::DLSNetwork() {
    _timeClient = new NTPClient(_ntpUDP, "pool.ntp.org", 0, 60000);
    _lastReconnectAttempt = 0;
    _ledPin = -1;
    _txPower = WIFI_POWER_8_5dBm;
}

void DLSNetwork::setTxPower(wifi_power_t power) {
    _txPower = power;
    if (isConnected()) {
        WiFi.setTxPower(_txPower);
    }
}

void DLSNetwork::onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("[WiFi] Station Started");
            WiFi.setTxPower(_txPower); // Ensure power is set when STA starts
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("[WiFi] Connected to Access Point");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("[WiFi] Got IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("[WiFi] Tx Power: ");
            Serial.println(WiFi.getTxPower());
            if (_ledPin != -1) digitalWrite(_ledPin, HIGH);
            _timeClient->begin();
            _timeClient->forceUpdate();
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[WiFi] Disconnected");
            if (_ledPin != -1) digitalWrite(_ledPin, LOW);
            break;
        default:
            break;
    }
}

void DLSNetwork::begin(String ssid, String pass, int ledPin) {
    _ssid = ssid;
    _pass = pass;
    _ledPin = ledPin;

    if (_ledPin != -1) {
        pinMode(_ledPin, OUTPUT);
        digitalWrite(_ledPin, LOW);
    }

    Serial.print("\n[WiFi] Configuring Ent-driven Connection to: "); Serial.println(_ssid);

    // Register Event Handler
    WiFi.onEvent(std::bind(&DLSNetwork::onWiFiEvent, this, std::placeholders::_1));

    // Professional Setup as requested
    WiFi.mode(WIFI_STA);
    WiFi.persistent(false);      // Don't save credentials to Flash
    WiFi.setAutoReconnect(true); // Let ESP32 background task handle reconnection
    
    WiFi.begin(_ssid.c_str(), _pass.c_str());
    
    // Attempt to set power (also handled in STA_START event for robustness)
    WiFi.setTxPower(_txPower);
}

void DLSNetwork::update() {
    // Stability Logic
    if (WiFi.status() == WL_CONNECTED) {
         if (_ledPin != -1) digitalWrite(_ledPin, HIGH);
         _timeClient->update();
         _lastReconnectAttempt = millis(); 
    } else {
        if (_ledPin != -1) digitalWrite(_ledPin, LOW);
        
        // With AutoReconnect(true), we rarely need manual intervention.
        // However, if the stack gets stuck or logic fails (e.g. 5 mins disconnected), we reboot.
        // We can also try a manual reconnect pulse if truly stuck but not reboot-worthy,
        // but user specifically preferred AutoReconnect system.
        
        // If disconnected for more than 5 minutes, restart ESP
        if (millis() - _lastReconnectAttempt > 300000) {
            Serial.println("[WiFi] Stuck disconnected for 5 mins. Restarting...");
            delay(1000);
            ESP.restart();
        }
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
        MDNS.addService("http", "tcp", 80);
        MDNS.addService("dls_weather", "udp", 12345); 
    } else {
        Serial.println("[mDNS] Failed to start!");
    }
}
