#include "WebUI.h"

#include <ArduinoJson.h>
#include <esp_random.h>

#include "version.h"
#include "WebUIHtml.h"

// ---------------------------------------------------------------------------
// Yardimcilar
// ---------------------------------------------------------------------------
String WebUI::esc(const String &in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            case '\'': out += "&#39;"; break;
            default:
                if ((uint8_t)c >= 0x20) out += c; // kontrol karakterlerini at
                break;
        }
    }
    return out;
}

String WebUI::intervalOptions(int current) {
    static const int vals[] = {5, 10, 15, 20, 30, 60, 120, 240};
    String out;
    bool found = false;
    for (unsigned i = 0; i < sizeof(vals) / sizeof(vals[0]); i++) {
        if (vals[i] == current) found = true;
        out += "<option value='" + String(vals[i]) + "'" +
               (vals[i] == current ? " selected" : "") + ">" + String(vals[i]) + " dk</option>";
    }
    if (!found) {
        out += "<option value='" + String(current) + "' selected>" + String(current) + " dk</option>";
    }
    return out;
}

String WebUI::txPowerOptions(int current) {
    static const int vals[] = {78, 68, 60, 52, 44, 34, 26, 8};
    static const char *labels[] = {"19.5 dBm (maks)", "17 dBm", "15 dBm", "13 dBm",
                                   "11 dBm", "8.5 dBm (onerilen)", "6.5 dBm", "2 dBm (min)"};
    String out;
    for (unsigned i = 0; i < sizeof(vals) / sizeof(vals[0]); i++) {
        out += "<option value='" + String(vals[i]) + "'" +
               (vals[i] == current ? " selected" : "") + ">" + labels[i] + "</option>";
    }
    return out;
}

void WebUI::sendJson(int code, const String &body) {
    _server.sendHeader("Cache-Control", "no-store");
    _server.sendHeader("X-Content-Type-Options", "nosniff");
    _server.send(code, "application/json", body);
}

void WebUI::sendApiError(int code, const char *msg) {
    JsonDocument doc;
    doc["ok"] = false;
    doc["error"] = msg;
    String out;
    serializeJson(doc, out);
    sendJson(code, out);
}

// ---------------------------------------------------------------------------
// Guvenlik katmani
// ---------------------------------------------------------------------------
bool WebUI::hostAllowed() {
    String host = _server.hostHeader();
    int colon = host.indexOf(':');
    if (colon > 0) host = host.substring(0, colon);
    host.toLowerCase();
    host.trim();
    if (host.length() == 0) return true; // HTTP/1.0 istemcileri
    if (host.endsWith(".local")) return true; // mDNS
    bool isIp = true;
    for (size_t i = 0; i < host.length(); i++) {
        if (!isdigit(host[i]) && host[i] != '.') {
            isIp = false;
            break;
        }
    }
    if (isIp) return true;
    if (_cfg && host == _cfg->getHostname()) return true;
    return false; // DNS rebinding / yabanci alan adi
}

bool WebUI::originAllowed() {
    String origin = _server.header("Origin");
    if (origin.length() == 0) return true; // curl / seri araclar
    int scheme = origin.indexOf("://");
    String originHost = (scheme >= 0) ? origin.substring(scheme + 3) : origin;
    int slash = originHost.indexOf('/');
    if (slash > 0) originHost = originHost.substring(0, slash);

    String host = _server.hostHeader();
    originHost.toLowerCase();
    host.toLowerCase();
    return originHost == host;
}

bool WebUI::isAuthed() {
    if (_token == 0) return false;
    if ((uint32_t)(millis() - _tokenCreated) > 12UL * 3600UL * 1000UL) return false;

    String cookie = _server.header("Cookie");
    if (cookie.length() == 0) return false;
    int idx = cookie.indexOf("dls_sid=");
    if (idx < 0) return false;
    String value = cookie.substring(idx + 8);
    int end = value.indexOf(';');
    if (end >= 0) value = value.substring(0, end);
    value.trim();
    char expected[9];
    snprintf(expected, sizeof(expected), "%08x", (unsigned)_token);
    return value.equalsIgnoreCase(expected);
}

void WebUI::setSessionCookie() {
    char buf[96];
    snprintf(buf, sizeof(buf), "dls_sid=%08x; Path=/; HttpOnly; SameSite=Strict; Max-Age=43200",
             (unsigned)_token);
    _server.sendHeader("Set-Cookie", buf);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------
String WebUI::renderLogin(const String &err) {
    String page = String(WEBUI_LOGIN);
    page.replace("%HOST%", esc(_cfg ? _cfg->getHostname() : String("dls-weather")));
    if ((int32_t)(millis() - _lockUntil) < 0) {
        page.replace("%ERROR%", esc("Cok fazla hatali deneme. 1 dakika bekleyin."));
    } else {
        page.replace("%ERROR%", esc(err));
    }
    return page;
}

String WebUI::renderPage() {
    String page = String(WEBUI_PAGE);
    page.reserve(page.length() + 700);

    String devName = _cfg->getAlias().length() ? _cfg->getAlias() : _cfg->getStationID();
    page.replace("%DEVNAME%", esc(devName));
    page.replace("%STATION%", esc(_cfg->getStationID()));
    String ipText = (_net && _net->isConnected()) ? WiFi.localIP().toString() : String("-");
    page.replace("%IP%", ipText);
    page.replace("%SSID%", esc(_cfg->getSSID()));
    page.replace("%STATIONV%", esc(_cfg->getStationID()));
    page.replace("%ALIAS%", esc(_cfg->getAlias()));
    page.replace("%LAT%", String(_cfg->getLat(), 6));
    page.replace("%LON%", String(_cfg->getLon(), 6));
    page.replace("%PASSPH%", _cfg->getPass().length() ? "kayıtlı şifre" : "açık ağ için boş");
    page.replace("%APIPH%", _cfg->getAPIKey().length() ? "kayıtlı anahtar" : "dls_...");
    page.replace("%INTERVAL_OPTS%", intervalOptions(_cfg->getInterval()));
    page.replace("%TX_OPTS%", txPowerOptions(_cfg->getTxPower()));
    page.replace("%DS_CHECKED%", _cfg->isDeepSleepEnabled() ? "checked" : "");
    page.replace("%SENSORS%", esc(_sensors.length() ? _sensors : String("-")));
    page.replace("%VERSION%", DLS_FW_VERSION);
    return page;
}

// ---------------------------------------------------------------------------
// Rotalar
// ---------------------------------------------------------------------------
void WebUI::begin(Config *cfg, DLSNetwork *net) {
    _cfg = cfg;
    _net = net;
    _token = esp_random();
    _tokenCreated = millis();

    const char *headers[] = {"Origin", "Cookie", "Host"};
    _server.collectHeaders(headers, 3);

    route();
    _server.begin();
    Serial.println("[Web] Arayuz hazir (PIN korumali).");
}

void WebUI::route() {
    _server.on("/", HTTP_GET, [this]() { onRoot(); });
    _server.on("/login", HTTP_GET, [this]() { onLoginGet(); });
    _server.on("/login", HTTP_POST, [this]() { onLoginPost(); });
    _server.on("/save", HTTP_POST, [this]() { onSave(); });
    _server.on("/api/weather", HTTP_GET, [this]() { onApiWeather(); });
    _server.on("/api/status", HTTP_GET, [this]() { onApiStatus(); });
    _server.on("/restart", HTTP_POST, [this]() { onRestart(); });
    _server.on("/favicon.ico", HTTP_GET, [this]() { _server.send(204, "text/plain", ""); });
    _server.onNotFound([this]() {
        if (!hostAllowed()) return sendApiError(403, "forbidden host");
        sendApiError(404, "not found");
    });
}

void WebUI::handle() {
    _server.handleClient();
    if (restartPending()) {
        Serial.println("[Web] Yeniden baslatiliyor...");
        delay(150);
        ESP.restart();
    }
}

void WebUI::onRoot() {
    if (!hostAllowed()) return sendApiError(403, "forbidden host");
    if (!isAuthed()) {
        _server.sendHeader("Location", "/login");
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(302, "text/plain", "");
        return;
    }
    String page = renderPage();
    _server.sendHeader("Cache-Control", "no-store");
    _server.sendHeader("X-Content-Type-Options", "nosniff");
    _server.sendHeader("X-Frame-Options", "DENY");
    _server.sendHeader("Referrer-Policy", "no-referrer");
    _server.sendHeader("Content-Security-Policy",
                       "default-src 'none'; style-src 'unsafe-inline'; script-src 'unsafe-inline'; "
                       "connect-src 'self'; img-src 'self' data:");
    _server.send(200, "text/html; charset=utf-8", page);
}

void WebUI::onLoginGet() {
    if (!hostAllowed()) return sendApiError(403, "forbidden host");
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(200, "text/html; charset=utf-8", renderLogin(""));
}

void WebUI::onLoginPost() {
    if (!hostAllowed()) return sendApiError(403, "forbidden host");
    if (!originAllowed()) return sendApiError(403, "cross-origin");

    // Basit kaba kuvvet kilidi
    if ((int32_t)(millis() - _lockUntil) < 0) {
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(429, "text/html; charset=utf-8", renderLogin(""));
        return;
    }

    String pin = _server.arg("pin");
    pin.trim();
    if (Config::validPin(pin) && pin == _cfg->getWebPin()) {
        _failCount = 0;
        _token = esp_random();
        _tokenCreated = millis();
        setSessionCookie();
        _server.sendHeader("Location", "/");
        _server.sendHeader("Cache-Control", "no-store");
        _server.send(302, "text/plain", "");
        return;
    }

    if (_failCount < 255) _failCount++;
    if (_failCount >= 5) {
        _lockUntil = millis() + 60000;
        _failCount = 0;
    }
    _server.sendHeader("Cache-Control", "no-store");
    _server.send(401, "text/html; charset=utf-8", renderLogin("PIN hatali."));
}

void WebUI::onSave() {
    if (!hostAllowed()) return sendApiError(403, "forbidden host");
    if (!isAuthed()) return sendApiError(401, "auth required");
    if (!originAllowed()) return sendApiError(403, "cross-origin");

    if (!_server.hasArg("ssid") || !_server.hasArg("station") || !_server.hasArg("lat") ||
        !_server.hasArg("lon")) {
        return sendApiError(400, "eksik alan");
    }

    ConfigInput in;
    in.ssid = _server.arg("ssid");
    in.stationId = _server.arg("station");
    in.alias = _server.arg("alias");
    // Bos birakilan sirlar mevcut degeri korur
    in.pass = _server.arg("pass").length() ? _server.arg("pass") : _cfg->getPass();
    in.apiKey = _server.arg("api").length() ? _server.arg("api") : _cfg->getAPIKey();

    String latStr = _server.arg("lat");
    String lonStr = _server.arg("lon");
    latStr.replace(',', '.');
    lonStr.replace(',', '.');
    in.lat = latStr.toFloat();
    in.lon = lonStr.toFloat();
    if (latStr.length() == 0 || lonStr.length() == 0) return sendApiError(400, "konum bos");

    in.interval = _server.arg("interval").toInt();
    in.txPower = _server.arg("txPower").toInt();
    in.deepSleep = _server.hasArg("deepSleep");

    String newPin = _server.arg("webpin");
    newPin.trim();
    if (newPin.length() > 0 && !Config::validPin(newPin)) {
        return sendApiError(400, "PIN 6 haneli olmali");
    }

    String err;
    if (!_cfg->apply(in, err)) return sendApiError(400, err.c_str());
    if (newPin.length() > 0) _cfg->setWebPin(newPin);

    sendJson(200, "{\"ok\":true}");
    _restartAt = millis() + 1200;
}

void WebUI::onApiWeather() {
    JsonDocument doc;
    doc["status"] = true;
    if (_air.valid) {
        if (isfinite(_air.temperature)) doc["temperature"] = _air.temperature;
        if (isfinite(_air.humidity)) doc["humidity"] = _air.humidity;
        if (isfinite(_air.pressure)) doc["pressure"] = _air.pressure;
        if (isfinite(_air.gasResistance) && _air.gasResistance > 0) doc["air_quality"] = _air.gasResistance;
    }
    if (_light.valid && isfinite(_light.uvIndex)) doc["uv_index"] = _light.uvIndex;
    if (_wind.valid) {
        if (isfinite(_wind.speed)) doc["wind_speed"] = _wind.speed;
        if (isfinite(_wind.direction)) doc["wind_dir"] = _wind.direction;
    }
    if (_rain.valid) {
        if (isfinite(_rain.rate)) doc["rain_rate"] = _rain.rate;
        if (isfinite(_rain.daily)) doc["rain_daily"] = _rain.daily;
    }
    if (_battValid) {
        if (isfinite(_battV)) doc["voltage"] = _battV;
        doc["battery"] = _battPct;
    }
    doc["last_send"] = _lastSend;

    String out;
    serializeJson(doc, out);
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    sendJson(200, out);
}

void WebUI::onApiStatus() {
    if (!hostAllowed()) return sendApiError(403, "forbidden host");
    if (!isAuthed()) return sendApiError(401, "auth required");

    JsonDocument doc;
    doc["version"] = DLS_FW_VERSION;
    doc["uptime"] = millis() / 1000UL;
    doc["wifi"] = _net && _net->isConnected();
    doc["rssi"] = _net ? _net->getRSSI() : 0;
    doc["ip"] = _net && _net->isConnected() ? WiFi.localIP().toString() : String("");
    doc["time_synced"] = _net && _net->timeSynced();
    doc["heap"] = ESP.getFreeHeap();
    doc["lastSend"] = _lastSend;
    doc["bat"] = _battValid ? _battPct : -1;
    doc["interval"] = _cfg ? _cfg->getInterval() : 0;
    doc["deepSleep"] = _cfg ? _cfg->isDeepSleepEnabled() : false;
    doc["sensors"] = _sensors;

    String out;
    serializeJson(doc, out);
    sendJson(200, out);
}

void WebUI::onRestart() {
    if (!hostAllowed()) return sendApiError(403, "forbidden host");
    if (!isAuthed()) return sendApiError(401, "auth required");
    if (!originAllowed()) return sendApiError(403, "cross-origin");
    sendJson(200, "{\"ok\":true}");
    _restartAt = millis() + 800;
}

void WebUI::updateSnapshot(const AirData &air, const LightData &light, const WindData &wind,
                           const RainData &rain, bool battValid, float battVoltage, int battPct) {
    _air = air;
    _light = light;
    _wind = wind;
    _rain = rain;
    _battValid = battValid;
    _battV = battVoltage;
    _battPct = battPct;
}
