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

class Display {
public:
    Display();
    void begin(TwoWire *wire = &Wire);

    // Screens
    void printStartup(String ssid);
    void printStatus(String ip, String stationId, String timeStr);
    void printData(float temp, float hum, float press, float uv, String statusMsg);
    void showMessage(String msg);

private:
    DisplayType _type;
    Adafruit_SSD1306* _ssd1306;
    Adafruit_SH1106G* _sh1106;
    
    void clear();
    void display();
    void setCursor(int x, int y);
    void setTextSize(int s);
    void print(String s);
    void print(float f, int dec = 1);
    void println(String s);
};
