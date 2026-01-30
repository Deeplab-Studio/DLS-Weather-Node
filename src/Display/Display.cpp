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
    Serial.println("\n[Display] Taramasi Baslatiliyor...");
    
    // 1. Try SSD1306
    _ssd1306 = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, wire, OLED_RESET);
    if (_ssd1306->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        _type = DISP_SSD1306;
        Serial.println("[Display] SSD1306 (0x3C) Tespit Edildi!");
        _ssd1306->clearDisplay();
        _ssd1306->setTextColor(SSD1306_WHITE);
        _ssd1306->display();
        return;
    } 
    delete _ssd1306;
    _ssd1306 = nullptr;

    // 2. Try SH1106 (Requires SH110X lib)
    // Note: SH1106G constructor might accept address on begin, or we set it.
    _sh1106 = new Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, wire, OLED_RESET);
    if (_sh1106->begin(0x3C, true)) { // Try 0x3C
        _type = DISP_SH1106;
        Serial.println("[Display] SH1106 (0x3C) Tespit Edildi!");
        _sh1106->clearDisplay();
        _sh1106->setTextColor(SH110X_WHITE);
        _sh1106->display();
        return;
    }
    delete _sh1106;
    _sh1106 = nullptr;

    Serial.println("[Display] HICBIR EKRAN BULUNAMADI.");
}

// --- Wrapper Methods to abstract differences ---

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

// --- Specific Screens ---

void Display::printStartup(String ssid) {
    if (_type == DISP_NONE) return;
    clear();
    setTextSize(1);
    setCursor(0, 0);
    println("DLS Weather Boot");
    println("----------------");
    println("Wi-Fi Baglaniyor:");
    println(ssid);
    display();
}

void Display::printStatus(String ip, String stationId, String timeStr) {
    if (_type == DISP_NONE) return;
    clear();
    setTextSize(1);
    setCursor(0, 0);
    print("IP: "); println(ip);
    print("ID: "); println(stationId);
    println("----------------");
    setCursor(0, 25);
    setTextSize(2);
    println(timeStr);
    display();
}

void Display::printData(float temp, float hum, float press, float uv, String statusMsg) {
    if (_type == DISP_NONE) return;
    clear();
    setTextSize(1);
    setCursor(0, 0);
    
    // Row 1: Temp & Hum
    print("T: "); print(temp, 1); print("C ");
    print("H: "); print(hum, 0); println("%");
    
    // Row 2: Press & UV
    print("P: "); print(press, 0); print("mb");
    if (uv >= 0) {
       print(" UV:"); print(uv, 1);
    }
    println("");

    // Row 3: Status
    println("----------------");
    print("Stat: "); println(statusMsg);
    
    display();
}

void Display::showMessage(String msg) {
    if (_type == DISP_NONE) return;
    clear();
    setTextSize(1);
    setCursor(0, 0);
    println(msg);
    display();
}
