#include "Display.h"

Display::Display() {
    _oled = new AutoOLED(128, 64, -1);
}

void Display::begin(TwoWire *wire) {
    Serial.println("\n[Display] Scanning...");
    
    // AutoOLED handles everything!
    if (_oled->begin(wire)) {
        Serial.print("[Display] Found: ");
        Serial.println(_oled->getType() == OLED_SSD1306 ? "SSD1306" : "SH1106");
        return;
    }

    Serial.println("[Display] NO DISPLAY FOUND.");
}

void Display::update() {
    if (_oled->getType() == OLED_NONE) return;

    if (millis() - _lastSwitchTime > _pageDuration) {
        int next = (int)_currentPage + 1;
        if (next >= PAGE_COUNT) next = 0;
        _currentPage = (DisplayPage)next;
        _lastSwitchTime = millis();
    }

    _oled->clearDisplay();
    
    // Page Order: NET -> AIR -> RAIN -> WIND -> LIGHT
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

// --- Data Setters ---
void Display::setAirData(float temp, float hum, float pres, float gas) {
    _airData.temp = temp;
    _airData.hum = hum;
    _airData.pres = pres;
    _airData.gas = gas;
    _airData.valid = true;
}

void Display::setWindData(float speed, float dir) {
    _windData.speed = speed;
    _windData.dir = dir;
    _windData.valid = true;
}

void Display::setRainData(float rate, float daily) {
    _rainData.rate = rate;
    _rainData.daily = daily;
    _rainData.valid = true;
}

void Display::setBatteryData(float voltage, int percentage) {
    _batteryData.voltage = voltage;
    _batteryData.percentage = percentage;
    _batteryData.valid = true;
}

void Display::setLightData(float uv, float lux) {
    _lightData.uv = uv;
    _lightData.lux = lux;
    _lightData.valid = true;
}

void Display::setNetworkInfo(String ip, String ssid, String status, bool connected) {
    _netData.ip = ip;
    _netData.ssid = ssid;
    _netData.status = status;
    _netData.connected = connected;
}

// --- Drawing Pages ---

void Display::drawCenteredHeader(String title) {
    _oled->setTextSize(1);
    int charWidth = 6; // Approx for size 1
    int textWidth = title.length() * charWidth;
    int xStart = (128 - textWidth) / 2;
    if (xStart < 0) xStart = 0;

    // Draw Text
    _oled->setCursor(xStart, 0);
    _oled->print(title);

    // Draw Lines
    int lineY = 3; // Middle of char height approx
    // Left Line
    if (xStart > 5) {
        _oled->drawLine(0, lineY, xStart - 3, lineY, SSD1306_WHITE);
    }
    // Right Line
    int xEnd = xStart + textWidth;
    if (xEnd < 128 - 5) {
        _oled->drawLine(xEnd + 3, lineY, 128, lineY, SSD1306_WHITE);
    }
}

void Display::drawNetPage() {
    drawCenteredHeader("NETWORK");

    _oled->setTextSize(1);
    _oled->setCursor(0, 15);
    _oled->print("SSID: "); _oled->println(_netData.ssid);

    _oled->setCursor(0, 28);
    _oled->print("IP:   "); _oled->println(_netData.ip);
}

void Display::drawAirPage() {
    drawCenteredHeader("WEATHER");
    _oled->setTextSize(1);
    
    // Temp
    _oled->setCursor(0, 12);
    _oled->print("Temp: ");
    if (_airData.temp != -999.0) { _oled->print(_airData.temp, 1); _oled->println(" C"); }
    else _oled->println("NaN");

    // Hum
    _oled->setCursor(0, 22);
    _oled->print("Hum:  ");
    if (_airData.hum != -999.0) { _oled->print(_airData.hum, 0); _oled->println(" %"); }
    else _oled->println("NaN");

    // Pres
    _oled->setCursor(0, 32);
    _oled->print("Pres: ");
    if (_airData.pres != -999.0) { _oled->print(_airData.pres, 0); _oled->println(" hPa"); }
    else _oled->println("NaN");

    // IAQ / Gas
    _oled->setCursor(0, 42);
    _oled->print("IAQ:  "); // Changed from Gas to IAQ
    if (_airData.gas != -999.0 && _airData.gas > 0) { _oled->print(_airData.gas / 1000.0, 1); _oled->println(" kOhm"); }
    else _oled->println("NaN");
}

void Display::drawRainPage() {
    drawCenteredHeader("RAIN");
    _oled->setTextSize(1);
    
    _oled->setCursor(0, 15);
    _oled->print("Rate:  ");
    if (_rainData.valid && _rainData.rate != -1.0) { _oled->print(_rainData.rate, 1); _oled->println(" mm/h"); }
    else _oled->println("NaN");

    _oled->setCursor(0, 30);
    _oled->print("Daily: ");
    if (_rainData.valid && _rainData.daily != -1.0) { _oled->print(_rainData.daily, 1); _oled->println(" mm"); }
    else _oled->println("NaN");
}

void Display::drawWindPage() {
    drawCenteredHeader("WIND");
    _oled->setTextSize(1);

    _oled->setCursor(0, 15);
    _oled->print("Speed: ");
    if (_windData.valid && _windData.speed != -1.0) { _oled->print(_windData.speed, 1); _oled->println(" m/s"); }
    else _oled->println("NaN");

    _oled->setCursor(0, 30);
    _oled->print("Dir:   ");
    if (_windData.valid && _windData.dir != -1.0) { _oled->print(_windData.dir, 0); _oled->println(" dg"); }
    else _oled->println("NaN");
}

void Display::drawLightPage() {
    drawCenteredHeader("UV / LIGHT");
    _oled->setTextSize(1);

    _oled->setCursor(0, 15);
    _oled->print("UV Index: ");
    if (_lightData.valid && _lightData.uv != -1.0) _oled->print(_lightData.uv, 1);
    else _oled->print("NaN");

    _oled->setCursor(0, 30);
    _oled->print("Light:    ");
    if (_lightData.valid && _lightData.lux != -1.0) { _oled->print(_lightData.lux, 0); _oled->println(" lx"); }
    else _oled->println("NaN");
}

// --- New UI Methods ---
void Display::setStatus(String status, bool isError) {
    _statusMsg = status;
    _isStatusError = isError;
    _statusTime = millis();
}

void Display::drawWifiIcon(int x, int y, bool connected) {
    if (connected) {
         _oled->fillCircle(x+6, y+6, 2, SSD1306_WHITE);
    } else {
        // Empty circle for disconnected
        _oled->drawCircle(x+6, y+6, 2, SSD1306_WHITE);
    }
}

void Display::drawFooter() {
    // Footer line
    _oled->drawLine(0, 54, 128, 54, SSD1306_WHITE);

    _oled->setTextSize(1);
    
    // Left: Status Msg (e.g. "Sending...", "Success!", "HTTP:403")
    _oled->setCursor(0, 56);
    if (_isStatusError) {
        _oled->print("ERR: "); _oled->print(_statusMsg);
    } else {
        if (_statusMsg.length() > 0) {
             _oled->print(_statusMsg);
        } else {
             // Default if empty?
             _oled->print("Stat: "); _oled->print(_netData.status); 
        }
    }
    
    // Right: WiFi Icon
    // Screen width 128. Icon ~12px wide. 
    // Position: 115, 55
    drawWifiIcon(116, 55, _netData.connected);
}

// --- Helpers & Existing Wrappers ---

void Display::printStartup(String ssid) {
    if (_oled->getType() == OLED_NONE) return;
    _oled->clearDisplay();
    
    // Centered "DLS Weather Station" (Roughly)
    // 128 px wide. Char width ~6px (size 1). 
    // "DLS Weather" = 11 chars * 6 = 66 px. (128-66)/2 = 31
    // "Station" = 7 chars * 6 = 42 px. (128-42)/2 = 43
    
    _oled->setTextSize(1); 
    
    _oled->setCursor(30, 15);
    _oled->println("DLS Weather");
    _oled->setCursor(40, 28);
    _oled->println("Station");
    
    // Status at bottom
    _oled->setCursor(0, 50);
    _oled->print("WiFi: "); _oled->println(ssid);
    
    _oled->display();
}

void Display::showMessage(String msg) {
    if (_oled->getType() == OLED_NONE) return;
    _oled->clearDisplay();
    _oled->setTextSize(1); _oled->setCursor(0, 0);
    _oled->println(msg);
    _oled->display();
}

void Display::off() {
    if (_oled->getType() == OLED_NONE) return;
    _oled->clearDisplay();
    _oled->display(); // Make it black
    _oled->off(); // Hardware off
}

void Display::on() {
    if (_oled->getType() == OLED_NONE) return;
    _oled->on(); // Hardware on
    update(); // Force a redraw to "turn on"
}
