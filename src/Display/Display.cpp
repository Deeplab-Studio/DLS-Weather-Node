#include "Display.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Display::Display() {
    _type = DISP_NONE;
    _ssd1306 = nullptr;
    _sh1106 = nullptr;
}

void Display::begin(TwoWire *wire) {
    Serial.println("\n[Display] Scanning...");
    
    _ssd1306 = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, wire, OLED_RESET);
    if (_ssd1306->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        _type = DISP_SSD1306;
        Serial.println("[Display] SSD1306 (0x3C) Found!");
        _ssd1306->clearDisplay();
        _ssd1306->setTextColor(SSD1306_WHITE);
        _ssd1306->display();
        return;
    } 
    delete _ssd1306; _ssd1306 = nullptr;

    _sh1106 = new Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, wire, OLED_RESET);
    if (_sh1106->begin(0x3C, true)) {
        _type = DISP_SH1106;
        Serial.println("[Display] SH1106 (0x3C) Found!");
        _sh1106->clearDisplay();
        _sh1106->setTextColor(SH110X_WHITE);
        _sh1106->display();
        return;
    }
    delete _sh1106; _sh1106 = nullptr;

    Serial.println("[Display] NO DISPLAY FOUND.");
}

void Display::update() {
    if (_type == DISP_NONE) return;

    if (millis() - _lastSwitchTime > _pageDuration) {
        int next = (int)_currentPage + 1;
        if (next >= PAGE_COUNT) next = 0;
        _currentPage = (DisplayPage)next;
        _lastSwitchTime = millis();
    }

    clear();
    
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
    display();
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

// --- Drawing Pages ---

void Display::drawCenteredHeader(String title) {
    setTextSize(1);
    int charWidth = 6; // Approx for size 1
    int textWidth = title.length() * charWidth;
    int xStart = (SCREEN_WIDTH - textWidth) / 2;
    if (xStart < 0) xStart = 0;

    // Draw Text
    setCursor(xStart, 0);
    print(title);

    // Draw Lines
    int lineY = 3; // Middle of char height approx
    // Left Line
    if (xStart > 5) {
        if (_type == DISP_SSD1306) _ssd1306->drawLine(0, lineY, xStart - 3, lineY, SSD1306_WHITE);
        else if (_type == DISP_SH1106) _sh1106->drawLine(0, lineY, xStart - 3, lineY, SH110X_WHITE);
    }
    // Right Line
    int xEnd = xStart + textWidth;
    if (xEnd < SCREEN_WIDTH - 5) {
        if (_type == DISP_SSD1306) _ssd1306->drawLine(xEnd + 3, lineY, SCREEN_WIDTH, lineY, SSD1306_WHITE);
        else if (_type == DISP_SH1106) _sh1106->drawLine(xEnd + 3, lineY, SCREEN_WIDTH, lineY, SH110X_WHITE);
    }
}

void Display::drawNetPage() {
    drawCenteredHeader("NETWORK");

    setTextSize(1);
    setCursor(0, 15);
    print("SSID: "); println(_netData.ssid);

    setCursor(0, 28);
    print("IP:   "); println(_netData.ip);
}

void Display::drawAirPage() {
    drawCenteredHeader("WEATHER");
    setTextSize(1);
    
    // Temp
    setCursor(0, 12);
    print("Temp: ");
    if (_airData.temp != -999.0) { print(_airData.temp, 1); println(" C"); }
    else println("NaN");

    // Hum
    setCursor(0, 22);
    print("Hum:  ");
    if (_airData.hum != -999.0) { print(_airData.hum, 0); println(" %"); }
    else println("NaN");

    // Pres
    setCursor(0, 32);
    print("Pres: ");
    if (_airData.pres != -999.0) { print(_airData.pres, 0); println(" hPa"); }
    else println("NaN");

    // IAQ / Gas
    setCursor(0, 42);
    print("IAQ:  "); // Changed from Gas to IAQ
    if (_airData.gas != -999.0 && _airData.gas > 0) { print(_airData.gas / 1000.0, 1); println(" kOhm"); }
    else println("NaN");
}

void Display::drawRainPage() {
    drawCenteredHeader("RAIN");
    setTextSize(1);
    
    setCursor(0, 15);
    print("Rate:  ");
    if (_rainData.valid && _rainData.rate != -1.0) { print(_rainData.rate, 1); println(" mm/h"); }
    else println("NaN");

    setCursor(0, 30);
    print("Daily: ");
    if (_rainData.valid && _rainData.daily != -1.0) { print(_rainData.daily, 1); println(" mm"); }
    else println("NaN");
}

void Display::drawWindPage() {
    drawCenteredHeader("WIND");
    setTextSize(1);

    setCursor(0, 15);
    print("Speed: ");
    if (_windData.valid && _windData.speed != -1.0) { print(_windData.speed, 1); println(" m/s"); }
    else println("NaN");

    setCursor(0, 30);
    print("Dir:   ");
    if (_windData.valid && _windData.dir != -1.0) { print(_windData.dir, 0); println(" dg"); }
    else println("NaN");
}

void Display::drawLightPage() {
    drawCenteredHeader("UV / LIGHT");
    setTextSize(1);

    setCursor(0, 15);
    print("UV Index: ");
    if (_lightData.valid && _lightData.uv != -1.0) print(_lightData.uv, 1);
    else print("NaN");

    setCursor(0, 30);
    print("Light:    ");
    if (_lightData.valid && _lightData.lux != -1.0) { print(_lightData.lux, 0); println(" lx"); }
    else println("NaN");
}

void Display::drawFooter() {
    // Footer line
    if (_type == DISP_SSD1306) _ssd1306->drawLine(0, 54, 128, 54, SSD1306_WHITE);
    else if (_type == DISP_SH1106) _sh1106->drawLine(0, 54, 128, 54, SH110X_WHITE);

    setTextSize(1);
    setCursor(0, 56);
    print("Stat: "); 
    println(_netData.status); // Use status string (Online/Offline) directly
}

// --- Helpers & Existing Wrappers ---

void Display::printStartup(String ssid) {
    if (_type == DISP_NONE) return;
    clear();
    
    // Centered "DLS Weather Station" (Roughly)
    // 128 px wide. Char width ~6px (size 1). 
    // "DLS Weather" = 11 chars * 6 = 66 px. (128-66)/2 = 31
    // "Station" = 7 chars * 6 = 42 px. (128-42)/2 = 43
    
    setTextSize(1); 
    
    setCursor(30, 15);
    println("DLS Weather");
    setCursor(40, 28);
    println("Station");
    
    // Status at bottom
    setCursor(0, 50);
    print("WiFi: "); println(ssid);
    
    display();
}

void Display::showMessage(String msg) {
    if (_type == DISP_NONE) return;
    clear();
    setTextSize(1); setCursor(0, 0);
    println(msg);
    display();
}

void Display::clear() {
    if (_type == DISP_SSD1306) _ssd1306->clearDisplay();
    else if (_type == DISP_SH1106) _sh1106->clearDisplay();
}

void Display::display() {
    if (_type == DISP_SSD1306) _ssd1306->display();
    else if (_type == DISP_SH1106) _sh1106->display();
}

void Display::setCursor(int x, int y) {
    if (_type == DISP_SSD1306) _ssd1306->setCursor(x, y);
    else if (_type == DISP_SH1106) _sh1106->setCursor(x, y);
}

void Display::setTextSize(int s) {
    if (_type == DISP_SSD1306) _ssd1306->setTextSize(s);
    else if (_type == DISP_SH1106) _sh1106->setTextSize(s);
}

void Display::print(String s) {
    if (_type == DISP_SSD1306) _ssd1306->print(s);
    else if (_type == DISP_SH1106) _sh1106->print(s);
}

void Display::print(float f, int dec) {
    if (_type == DISP_SSD1306) _ssd1306->print(f, dec);
    else if (_type == DISP_SH1106) _sh1106->print(f, dec);
}

void Display::println(String s) {
    if (_type == DISP_SSD1306) _ssd1306->println(s);
    else if (_type == DISP_SH1106) _sh1106->println(s);
}
