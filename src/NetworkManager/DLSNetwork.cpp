#include "DLSNetwork.h"

DLSNetwork::DLSNetwork() {
    _timeClient = new NTPClient(_ntpUDP, "pool.ntp.org", 0, 60000);
    _lastReconnectAttempt = 0;
    _ledPin = -1;
    _txPower = WIFI_POWER_8_5dBm;
}

bool DLSNetwork::isValidTxPower(int v) {
    switch (v) {
        case 78: case 68: case 60: case 52: case 44: case 34: case 26: case 8:
            return true;
        default:
            return false;
    }
}

void DLSNetwork::setTxPower(wifi_power_t power) {
    if (!isValidTxPower((int)power)) {
        Serial.println("[WiFi] Gecersiz TX guc degeri, 8.5 dBm kullanilacak.");
        power = WIFI_POWER_8_5dBm;
    }
    _txPower = power;
    if (isConnected()) WiFi.setTxPower(_txPower);
}

void DLSNetwork::setLed(bool on) {
    if (_ledPin < 0) return;
#if defined(LED_IS_RGB)
    // WS2812: duz digitalWrite ile rengi kontrol edilemez
    if (on) neopixelWrite(_ledPin, 0, 24, 0);
    else neopixelWrite(_ledPin, 0, 0, 0);
#else
    digitalWrite(_ledPin, on ? HIGH : LOW);
#endif
}

void DLSNetwork::onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_START:
            WiFi.setTxPower(_txPower);
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("[WiFi] Erisim noktasina baglandi.");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("[WiFi] IP: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("[WiFi] Baglanti koptu.");
            break;
        default:
            break;
    }
}

void DLSNetwork::begin(String ssid, String pass, int ledPin) {
    _ssid = ssid;
    _pass = pass;
    _ledPin = ledPin;

    if (_ledPin >= 0) {
#if defined(LED_IS_RGB)
        // neopixelWrite pini kendisi surer
#else
        pinMode(_ledPin, OUTPUT);
        digitalWrite(_ledPin, LOW);
#endif
        setLed(false);
    }

    Serial.print("\n[WiFi] Baglaniliyor: ");
    Serial.println(_ssid);

    WiFi.onEvent(std::bind(&DLSNetwork::onWiFiEvent, this, std::placeholders::_1));

    WiFi.mode(WIFI_STA);
    if (_hostname.length() > 0) {
        WiFi.setHostname(_hostname.c_str());
    }
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);

    WiFi.begin(_ssid.c_str(), _pass.c_str());
    WiFi.setTxPower(_txPower);

    _timeClient->begin();

    _lastReconnectAttempt = millis(); // 5 dk sayaci buradan baslar
}

bool DLSNetwork::waitConnected(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (WiFi.status() == WL_CONNECTED) return true;
        delay(100);
    }
    return WiFi.status() == WL_CONNECTED;
}

void DLSNetwork::update() {
    if (WiFi.status() == WL_CONNECTED) {
        setLed(true);

        if (!timeSynced()) {
            static unsigned long lastNtpRetry = 0;
            if (millis() - lastNtpRetry > 2000) {
                lastNtpRetry = millis();
                _timeClient->forceUpdate();
                if (timeSynced()) {
                    Serial.println("[NTP] Zaman alindi: " + _timeClient->getFormattedTime());
                }
            }
        } else {
            _timeClient->update();
        }

        _lastReconnectAttempt = millis();
    } else {
        setLed(false);
        // 5 dakika boyunca baglanilamazsa yeniden baslat
        if (millis() - _lastReconnectAttempt > 300000UL) {
            Serial.println("[WiFi] 5 dakikadir baglanti yok, yeniden baslatiliyor...");
            delay(200);
            ESP.restart();
        }
    }
}

bool DLSNetwork::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int DLSNetwork::getRSSI() {
    return isConnected() ? WiFi.RSSI() : 0;
}

unsigned long DLSNetwork::getEpochTime() {
    return _timeClient ? _timeClient->getEpochTime() : 0;
}

int DLSNetwork::getEpochDay() {
    unsigned long e = getEpochTime();
    return timeSynced() ? (int)(e / 86400UL) : 0;
}

bool DLSNetwork::timeSynced() {
    return getEpochTime() > 1600000000UL; // 2020'den sonrasi
}

int DLSNetwork::getMinutes() {
    return _timeClient ? _timeClient->getMinutes() : 0;
}

int DLSNetwork::getSeconds() {
    return _timeClient ? _timeClient->getSeconds() : 0;
}
