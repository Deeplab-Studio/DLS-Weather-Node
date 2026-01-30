#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SH110X.h>

enum DisplayType {
    DISP_NONE,
    DISP_SSD1306,
    DISP_SH1106
};

enum DisplayPage {
    PAGE_NET,
    PAGE_AIR,
    PAGE_RAIN,
    PAGE_WIND,
    PAGE_LIGHT,
    PAGE_COUNT
};

struct DispAirData {
    float temp = -999.0;
    float hum = -999.0;
    float pres = -999.0;
    float gas = -999.0; // IAQ / Gas Res
    bool valid = false;
};

struct DispWindData {
    float speed = -1.0;
    float dir = -1.0; // Direction in degrees
    bool valid = false;
};

struct DispRainData {
    float rate = -1.0;
    float daily = -1.0;
    bool valid = false;
};

struct DispLightData {
    float uv = -1.0;
    float lux = -1.0;
    bool valid = false;
};

struct DispNetData {
    String ip = "";
    String ssid = "";
    String status = "";
    bool connected = false;
};

class Display {
public:
    Display();
    void begin(TwoWire *wire = &Wire);
    void update(); // Main loop

    // Data Setters
    void setAirData(float temp, float hum, float pres, float gas);
    void setWindData(float speed, float dir); 
    void setRainData(float rate, float daily); 
    void setLightData(float uv, float lux);
    void setNetworkInfo(String ip, String ssid, String status, bool connected);
    void setStatus(String status, bool isError = false); // New Status Bar method

    void printStartup(String ssid);
    void showMessage(String msg);

    void off(); // Clear display and turn off
    void on();  // Restore/Turn on

private:
    DisplayType _type;
    Adafruit_SSD1306* _ssd1306;
    Adafruit_SH1106G* _sh1106;
    
    // Internal State
    DisplayPage _currentPage = PAGE_NET;
    unsigned long _lastSwitchTime = 0;
    const unsigned long _pageDuration = 5000; // 5 seconds

    // Data
    DispAirData _airData;
    DispWindData _windData;
    DispRainData _rainData;
    DispLightData _lightData;
    DispNetData _netData;

    // Status Bar
    String _statusMsg = "";
    bool _isStatusError = false;
    unsigned long _statusTime = 0;

    // Drawing Helpers
    void drawCenteredHeader(String title);
    void drawFooter();
    void drawWifiIcon(int x, int y, bool connected);
    void drawAirPage();
    void drawWindPage();
    void drawRainPage();
    void drawLightPage();
    void drawNetPage();

    // Hardware Wrappers
    void clear();
    void display();
    void setCursor(int x, int y);
    void setTextSize(int s);
    void print(String s);
    void print(float f, int dec = 1);
    void println(String s);
};
