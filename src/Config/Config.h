#pragma once

#include <Arduino.h>
#include <Preferences.h>

// --- Sinir degerleri (tek yerde) ---
#define CFG_SSID_MAX     32
#define CFG_PASS_MAX     64
#define CFG_APIKEY_MAX   128
#define CFG_STATION_MAX  32
#define CFG_ALIAS_MAX    32
#define CFG_PIN_LEN      6
#define CFG_INTERVAL_MIN 1
#define CFG_INTERVAL_MAX 1440

// Placeholder (fabrika) degerleri
#define CFG_DEF_SSID     "WIFI_SSID_GIRIN"
#define CFG_DEF_STATION  "STATION_ID"
#define CFG_DEF_APIKEY   "API_KEY"

// Kullanici girisinden gelen ham veri
struct ConfigInput {
    String ssid;
    String pass;
    String apiKey;
    String stationId;
    String alias;
    float  lat = 0.0f;
    float  lon = 0.0f;
    int    interval = 30;
    bool   deepSleep = false;
    int    txPower = 34; // WIFI_POWER_8_5dBm
};

class Config {
public:
    Config();
    void begin();
    void checkSerialCommands();

    // Dogrular, kirpar ve kaydeder. Gecersizse hicbir sey yazmaz -> false + err.
    bool apply(const ConfigInput &in, String &err);

    // Getters
    String getSSID() const { return _ssid; }
    String getPass() const { return _pass; }
    String getAPIKey() const { return _apiKey; }
    String getStationID() const { return _stationId; }
    String getAlias() const { return _alias; }
    float getLat() const { return _lat; }
    float getLon() const { return _lon; }
    int getInterval() const { return _intervalMin; }
    bool isDeepSleepEnabled() const { return _isDeepSleepEnabled; }
    int getTxPower() const { return _txPower; }
    String getWebPin() const { return _webPin; }
    void setWebPin(const String &pin);

    // Kurulum tamamlanmis mi? (placeholder degerlerden kurtulmus mu)
    bool isConfigured() const;

    // Yerel ag adi icin guvenli hostname ("dls-weather-<station>")
    String getHostname() const;

    // Gunluk yagis sayacinin yeniden baslatma/uyku sonrasi korunmasi
    float getStoredRainDaily() const { return _rainDaily; }
    int getStoredRainDay() const { return _rainDay; }
    void  setRainDaily(float mm, int epochDay);

    // --- Dogrulama yardimcilari (web + seri arayuz ayni kurallari kullanir) ---
    static bool validLat(float v) { return isfinite(v) && v >= -90.0f && v <= 90.0f; }
    static bool validLon(float v) { return isfinite(v) && v >= -180.0f && v <= 180.0f; }
    static bool validInterval(int v) { return v >= CFG_INTERVAL_MIN && v <= CFG_INTERVAL_MAX; }
    static bool validTxPower(int v);
    static bool validStationId(const String &s);
    static bool validPin(const String &s);
    static String sanitizeHostLabel(const String &s);

private:
    Preferences _prefs;

    String _ssid;
    String _pass;
    String _apiKey;
    String _stationId;
    String _alias;
    String _webPin;
    float _lat;
    float _lon;
    int _intervalMin;
    bool _isDeepSleepEnabled;
    int _txPower;      // wifi_power_t enum degeri
    float _rainDaily;  // son kaydedilen gunluk yagis (mm)
    int _rainDay;      // epoch gun numarasi

    void load();
    void info();
    bool persistAll();
    void ensurePin();
};
