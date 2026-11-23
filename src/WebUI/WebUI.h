#pragma once

#include <Arduino.h>
#include <WebServer.h>

#include "Config/Config.h"
#include "NetworkManager/DLSNetwork.h"
#include "Sensor/Sensor.h"

// Yerel web arayuzu: PIN ile korunan durum + ayar sayfasi ve JSON API.
class WebUI {
public:
    void begin(Config *cfg, DLSNetwork *net);
    void handle();

    // main() her okuma turunda cagirir
    void updateSnapshot(const AirData &air, const LightData &light, const WindData &wind,
                        const RainData &rain, bool battValid, float battVoltage, int battPct);
    void setSendStatus(const String &s) { _lastSend = s; }
    void setSensorsReport(const String &s) { _sensors = s; }

    bool restartPending() const { return _restartAt != 0 && (int32_t)(millis() - _restartAt) >= 0; }

private:
    WebServer _server{80};
    Config *_cfg = nullptr;
    DLSNetwork *_net = nullptr;

    uint32_t _token = 0;
    uint32_t _tokenCreated = 0;
    uint8_t _failCount = 0;
    uint32_t _lockUntil = 0;
    uint32_t _restartAt = 0;

    AirData _air;
    LightData _light;
    WindData _wind;
    RainData _rain;
    bool _battValid = false;
    float _battV = NAN;
    int _battPct = -1;
    String _lastSend = "";
    String _sensors = "";

    void route();
    void onRoot();
    void onSave();
    void onApiWeather();
    void onApiStatus();
    void onRestart();
    void onLoginGet();
    void onLoginPost();

    bool hostAllowed();
    bool originAllowed();
    bool isAuthed();
    void setSessionCookie();
    String renderPage();
    String renderLogin(const String &err);
    void sendJson(int code, const String &body);
    void sendApiError(int code, const char *msg);
    static String esc(const String &in);
    static String intervalOptions(int current);
    static String txPowerOptions(int current);
};
