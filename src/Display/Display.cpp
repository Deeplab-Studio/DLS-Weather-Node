#include "Display.h"

static const int SCREEN_W = 128;

Display::Display() {
    _oled = new AutoOLED(128, 64, -1);
}

void Display::begin(TwoWire *wire) {
    Serial.println("\n[Display] Taraniyor...");
    if (_oled->begin(wire)) {
        Serial.print("[Display] Bulundu: ");
        Serial.println(_oled->getType() == OLED_SSD1306 ? "SSD1306" : "SH1106");
        _dirty = true;
        return;
    }
    Serial.println("[Display] Ekran bulunamadi.");
}

String Display::clip(const String &s, size_t maxChars) {
    if (s.length() <= maxChars) return s;
    return s.substring(0, maxChars);
}

void Display::nextPage() {
    for (int i = 0; i < PAGE_COUNT; i++) {
        int next = ((int)_currentPage + 1 + i) % PAGE_COUNT;
        if (_available[next]) {
            _currentPage = (DisplayPage)next;
            return;
        }
    }
}

void Display::update() {
    if (!isPresent()) return;

    unsigned long now = millis();

    // Sayfa degisimi
    if (now - _lastSwitchTime > _pageDuration) {
        _lastSwitchTime = now;
        nextPage();
        _dirty = true;
    }

    // I2C yolunu her loop turunda mesgul etmemek icin cizimi kisitla
    if (!_dirty && (now - _lastDrawTime) < _minDrawInterval) return;
    _dirty = false;
    _lastDrawTime = now;

    _oled->clearDisplay();

    switch (_currentPage) {
        case PAGE_NET:   drawNetPage(); break;
        case PAGE_AIR:   drawAirPage(); break;
        case PAGE_RAIN:  drawRainPage(); break;
        case PAGE_WIND:  drawWindPage(); break;
        case PAGE_LIGHT: drawLightPage(); break;
        default: break;
    }

    drawFooter();
    _oled->display();
}

// --- Veri setter'lari ---
void Display::setAirData(float temp, float hum, float pres, float gas) {
    _airData.temp = isfinite(temp) ? temp : NAN;
    _airData.hum = isfinite(hum) ? hum : NAN;
    _airData.pres = isfinite(pres) ? pres : NAN;
    _airData.gas = isfinite(gas) ? gas : NAN;
    _airData.valid = isfinite(temp);
    _dirty = true;
}

void Display::setWindData(float speed, float dir) {
    _windData.speed = isfinite(speed) ? speed : NAN;
    _windData.dir = isfinite(dir) ? dir : NAN;
    _windData.valid = isfinite(speed) && isfinite(dir);
    _dirty = true;
}

void Display::setRainData(float rate, float daily) {
    _rainData.rate = isfinite(rate) ? rate : NAN;
    _rainData.daily = isfinite(daily) ? daily : NAN;
    _rainData.valid = isfinite(rate) && isfinite(daily);
    _dirty = true;
}

void Display::setLightData(float uv) {
    _lightData.uv = isfinite(uv) && uv >= 0 ? uv : NAN;
    _lightData.valid = isfinite(_lightData.uv);
    _dirty = true;
}

void Display::setBatteryData(float voltage, int percentage) {
    _batteryData.voltage = isfinite(voltage) ? voltage : NAN;
    _batteryData.percentage = percentage;
    _batteryData.valid = (percentage >= 0);
    _dirty = true;
}

void Display::setNetworkInfo(String ip, String ssid, String status, bool connected) {
    if (_netData.ip == ip && _netData.ssid == ssid && _netData.status == status &&
        _netData.connected == connected) {
        return;
    }
    _netData.ip = ip;
    _netData.ssid = ssid;
    _netData.status = status;
    _netData.connected = connected;
    _dirty = true;
}

void Display::setAvailableSensors(bool air, bool light, bool windRain) {
    _available[PAGE_NET] = true;
    _available[PAGE_AIR] = air;
    _available[PAGE_RAIN] = windRain;
    _available[PAGE_WIND] = windRain;
    _available[PAGE_LIGHT] = light;
    // Bulunulan sayfa artik gecerli degilse ilk gecerli sayfaya don
    if (!_available[_currentPage]) {
        _currentPage = PAGE_NET;
        _dirty = true;
    }
}

// --- Cizim ---
void Display::drawCenteredHeader(const String &title) {
    _oled->setTextSize(1);
    int textWidth = title.length() * 6;
    int xStart = (SCREEN_W - textWidth) / 2;
    if (xStart < 0) xStart = 0;

    _oled->setCursor(xStart, 0);
    _oled->print(title);

    int lineY = 3;
    if (xStart > 5) _oled->drawLine(0, lineY, xStart - 3, lineY, SSD1306_WHITE);
    int xEnd = xStart + textWidth;
    if (xEnd < SCREEN_W - 5) _oled->drawLine(xEnd + 3, lineY, SCREEN_W, lineY, SSD1306_WHITE);
}

void Display::drawNetPage() {
    drawCenteredHeader("NETWORK");
    _oled->setTextSize(1);

    _oled->setCursor(0, 14);
    _oled->print("SSID: ");
    _oled->println(clip(_netData.ssid, 15));

    _oled->setCursor(0, 26);
    _oled->print("IP:   ");
    _oled->println(_netData.ip);

    _oled->setCursor(0, 38);
    _oled->print("Web:  ");
    _oled->println(_netData.connected ? String("acik") : String("-"));

    if (_webPin.length() > 0) {
        _oled->setCursor(0, 48);
        _oled->print("PIN:  ");
        _oled->println(_webPin);
    }
}

void Display::drawAirPage() {
    drawCenteredHeader("WEATHER");
    _oled->setTextSize(1);

    _oled->setCursor(0, 12);
    _oled->print("Temp: ");
    if (isfinite(_airData.temp)) { _oled->print(_airData.temp, 1); _oled->println(" C"); }
    else _oled->println("--");

    _oled->setCursor(0, 22);
    _oled->print("Hum:  ");
    if (isfinite(_airData.hum)) { _oled->print(_airData.hum, 0); _oled->println(" %"); }
    else _oled->println("--");

    _oled->setCursor(0, 32);
    _oled->print("Pres: ");
    if (isfinite(_airData.pres)) { _oled->print(_airData.pres, 0); _oled->println(" hPa"); }
    else _oled->println("--");

    _oled->setCursor(0, 42);
    _oled->print("Gas:  ");
    if (isfinite(_airData.gas) && _airData.gas > 0) {
        _oled->print(_airData.gas, 1);
        _oled->println(" kOhm");
    } else {
        _oled->println("--");
    }
}

void Display::drawRainPage() {
    drawCenteredHeader("RAIN");
    _oled->setTextSize(1);

    _oled->setCursor(0, 15);
    _oled->print("Rate:  ");
    if (isfinite(_rainData.rate)) { _oled->print(_rainData.rate, 1); _oled->println(" mm/h"); }
    else _oled->println("--");

    _oled->setCursor(0, 30);
    _oled->print("Today: ");
    if (isfinite(_rainData.daily)) { _oled->print(_rainData.daily, 1); _oled->println(" mm"); }
    else _oled->println("--");
}

void Display::drawWindPage() {
    drawCenteredHeader("WIND");
    _oled->setTextSize(1);

    _oled->setCursor(0, 15);
    _oled->print("Speed: ");
    if (isfinite(_windData.speed)) { _oled->print(_windData.speed, 1); _oled->println(" m/s"); }
    else _oled->println("--");

    _oled->setCursor(0, 30);
    _oled->print("Dir:   ");
    if (isfinite(_windData.dir)) { _oled->print(_windData.dir, 0); _oled->println(" deg"); }
    else _oled->println("--");
}

void Display::drawLightPage() {
    drawCenteredHeader("UV");
    _oled->setTextSize(1);

    _oled->setCursor(0, 22);
    _oled->print("UV Index: ");
    if (isfinite(_lightData.uv)) _oled->println(_lightData.uv, 1);
    else _oled->println("--");
}

void Display::setStatus(String status, bool isError) {
    _statusMsg = status;
    _isStatusError = isError;
    _statusTime = millis();
    _dirty = true;
}

void Display::drawWifiIcon(int x, int y, bool connected) {
    if (connected) _oled->fillCircle(x + 4, y + 4, 3, SSD1306_WHITE);
    else _oled->drawCircle(x + 4, y + 4, 3, SSD1306_WHITE);
}

void Display::drawFooter() {
    _oled->drawLine(0, 53, SCREEN_W, 53, SSD1306_WHITE);
    _oled->setTextSize(1);

    // Durum mesaji 12 sn sonra temizlenir
    if (_statusMsg.length() > 0 && millis() - _statusTime > 12000) {
        _statusMsg = "";
        _isStatusError = false;
    }

    _oled->setCursor(0, 56);
    String left;
    if (_statusMsg.length() > 0) {
        left = (_isStatusError ? "! " : "") + _statusMsg;
    } else {
        left = _netData.connected ? "Online" : "Offline";
    }
    _oled->print(clip(left, 12));

    // Pil yuzdesi
    if (_batteryData.valid) {
        _oled->setCursor(84, 56);
        _oled->print(String(_batteryData.percentage) + "%");
    }

    drawWifiIcon(114, 54, _netData.connected);
}

// --- Baslangic / mesaj ---
void Display::printStartup(String ssid, String alias, String pin) {
    if (!isPresent()) return;
    _oled->clearDisplay();
    _oled->setTextSize(1);

    _oled->setCursor(26, 8);
    _oled->println("DLS Weather");
    _oled->setCursor(38, 20);
    _oled->println("Station");

    _oled->setCursor(0, 36);
    _oled->print("WiFi: ");
    _oled->println(clip(ssid, 15));

    if (alias.length() > 0) {
        _oled->setCursor(0, 46);
        _oled->print("Ad:   ");
        _oled->println(clip(alias, 15));
    }
    if (pin.length() > 0) {
        _oled->setCursor(0, 56);
        _oled->print("PIN:  ");
        _oled->println(pin);
    }

    _oled->display();
    _lastDrawTime = millis();
}

void Display::showMessage(String msg) {
    if (!isPresent()) return;
    _oled->clearDisplay();
    _oled->setTextSize(1);
    _oled->setCursor(0, 0);
    _oled->print(clip(msg, 21));
    _oled->display();
    _lastDrawTime = millis();
}

void Display::off() {
    if (!isPresent()) return;
    _oled->clearDisplay();
    _oled->display();
    _oled->off();
}

void Display::on() {
    if (!isPresent()) return;
    _oled->on();
    _dirty = true;
}
