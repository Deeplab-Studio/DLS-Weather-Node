#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <AutoOLED.h>

enum DisplayPage {
    PAGE_NET = 0,
    PAGE_AIR,
    PAGE_RAIN,
    PAGE_WIND,
    PAGE_LIGHT,
    PAGE_COUNT
};

// Tum veri alanlari NAN = "olcum yok" semantigini kullanir.
struct DispAirData {
    float temp = NAN;
    float hum = NAN;
    float pres = NAN;
    float gas = NAN; // kOhm (BME680)
    bool valid = false;
};

struct DispWindData {
    float speed = NAN;
    float dir = NAN;
    bool valid = false;
};

struct DispRainData {
    float rate = NAN;
    float daily = NAN;
    bool valid = false;
};

struct DispLightData {
    float uv = NAN;
    bool valid = false;
};

struct DispBatteryData {
    float voltage = NAN;
    int percentage = -1;
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
    void update(); // Ana dongu (kendi kendini kisitlar)

    // Veri setter'lari
    void setAirData(float temp, float hum, float pres, float gas);
    void setWindData(float speed, float dir);
    void setRainData(float rate, float daily);
    void setLightData(float uv);
    void setBatteryData(float voltage, int percentage);
    void setNetworkInfo(String ip, String ssid, String status, bool connected);
    void setStatus(String status, bool isError = false);
    void setWebPin(String pin) { _webPin = pin; }

    // Donanimda olmayan sayfalari donguden cikarir
    void setAvailableSensors(bool air, bool light, bool windRain);

    void printStartup(String ssid, String alias = "", String pin = "");
    void showMessage(String msg);

    void off(); // Ekrani temizle ve kapat
    void on();

    bool isPresent() const { return _oled && _oled->getType() != OLED_NONE; }

private:
    AutoOLED *_oled = nullptr;

    DisplayPage _currentPage = PAGE_NET;
    unsigned long _lastSwitchTime = 0;
    unsigned long _lastDrawTime = 0;
    const unsigned long _pageDuration = 5000;
    const unsigned long _minDrawInterval = 1000; // I2C yolunu mesgul etmemek icin
    bool _dirty = true;
    bool _available[PAGE_COUNT] = {true, true, true, true, true};

    DispAirData _airData;
    DispWindData _windData;
    DispRainData _rainData;
    DispLightData _lightData;
    DispBatteryData _batteryData;
    DispNetData _netData;

    String _statusMsg = "";
    bool _isStatusError = false;
    unsigned long _statusTime = 0;
    String _webPin = "";

    void nextPage();
    static String clip(const String &s, size_t maxChars);
    void drawCenteredHeader(const String &title);
    void drawFooter();
    void drawWifiIcon(int x, int y, bool connected);
    void drawAirPage();
    void drawWindPage();
    void drawRainPage();
    void drawLightPage();
    void drawNetPage();
};
