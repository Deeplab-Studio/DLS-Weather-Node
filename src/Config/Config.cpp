#include "Config.h"
#include <ArduinoJson.h>
#include <esp_random.h>

// Kabul edilen TX guc degerleri (wifi_power_t enum karsiliklari).
// WiFiGeneric.h: 78=19.5dBm, 68=17, 60=15, 52=13, 44=11, 34=8.5, 26=6.5, 8=2
static const int kValidTxPower[] = {78, 68, 60, 52, 44, 34, 26, 8};
static const int kValidTxPowerCount = sizeof(kValidTxPower) / sizeof(kValidTxPower[0]);

Config::Config() {
    _ssid = CFG_DEF_SSID;
    _pass = "";
    _apiKey = CFG_DEF_APIKEY;
    _stationId = CFG_DEF_STATION;
    _alias = "";
    _webPin = "";
    _lat = 0.0f;
    _lon = 0.0f;
    _intervalMin = 30;
    _isDeepSleepEnabled = false;
    _txPower = 34; // WIFI_POWER_8_5dBm (onceden 26 yaziyordu -> arayuzde 19.5dBm gorunuyordu)
    _rainDaily = 0.0f;
    _rainDay = 0;
}

void Config::begin() {
    if (!_prefs.begin("dls-config", false)) {
        Serial.println("[Config] NVS acilamadi! Varsayilan degerler kullanilacak.");
        return;
    }
    load();
    ensurePin();
}

void Config::load() {
    _ssid = _prefs.getString("ssid", _ssid);
    _pass = _prefs.getString("pass", _pass);
    _apiKey = _prefs.getString("api", _apiKey);
    _stationId = _prefs.getString("station", _stationId);
    _alias = _prefs.getString("alias", _alias);
    _lat = _prefs.getFloat("lat", _lat);
    _lon = _prefs.getFloat("lon", _lon);
    _intervalMin = _prefs.getInt("interval", _intervalMin);
    _isDeepSleepEnabled = _prefs.getBool("deepsleep", _isDeepSleepEnabled);
    _txPower = _prefs.getInt("txPower", _txPower);
    _webPin = _prefs.getString("webpin", _webPin);
    _rainDaily = _prefs.getFloat("raindaily", 0.0f);
    _rainDay = _prefs.getInt("rainday", 0);

    // Bozuk/eski NVS degerlerine karsi koruma
    if (!validInterval(_intervalMin)) _intervalMin = 30;
    if (!validTxPower(_txPower)) _txPower = 34;
    if (!validLat(_lat)) _lat = 0.0f;
    if (!validLon(_lon)) _lon = 0.0f;
    if (_ssid.length() > CFG_SSID_MAX) _ssid = _ssid.substring(0, CFG_SSID_MAX);
    if (_apiKey.length() > CFG_APIKEY_MAX) _apiKey = _apiKey.substring(0, CFG_APIKEY_MAX);
    if (_stationId.length() > CFG_STATION_MAX) _stationId = _stationId.substring(0, CFG_STATION_MAX);
    if (_alias.length() > CFG_ALIAS_MAX) _alias = _alias.substring(0, CFG_ALIAS_MAX);
}

void Config::ensurePin() {
    if (validPin(_webPin)) return;
    // 6 haneli, sifirla baslamayan rastgele PIN
    uint32_t r = esp_random() % 900000u;
    _webPin = String(100000u + r);
    if (_prefs.putString("webpin", _webPin) == 0) {
        Serial.println("[Config] UYARI: Web PIN NVS'e yazilamadi.");
    }
}

void Config::setWebPin(const String &pin) {
    if (!validPin(pin)) return;
    _webPin = pin;
    _prefs.putString("webpin", _webPin);
}

bool Config::validTxPower(int v) {
    for (int i = 0; i < kValidTxPowerCount; i++) {
        if (kValidTxPower[i] == v) return true;
    }
    return false;
}

bool Config::validStationId(const String &s) {
    if (s.length() == 0 || s.length() > CFG_STATION_MAX) return false;
    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '-' || c == '_';
        if (!ok) return false;
    }
    return true;
}

bool Config::validPin(const String &s) {
    if (s.length() != CFG_PIN_LEN) return false;
    for (size_t i = 0; i < s.length(); i++) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

String Config::sanitizeHostLabel(const String &s) {
    String out;
    for (size_t i = 0; i < s.length() && out.length() < 32; i++) {
        char c = s[i];
        if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
        bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
        if (ok) out += c;
    }
    while (out.startsWith("-")) out.remove(0, 1);
    while (out.endsWith("-")) out.remove(out.length() - 1);
    return out;
}

bool Config::isConfigured() const {
    return _ssid.length() > 0 && _ssid != CFG_DEF_SSID;
}

String Config::getHostname() const {
    String host = "dls-weather";
    String label = sanitizeHostLabel(isConfigured() ? _stationId : String(""));
    if (label.length() > 0 && label != "station_id") host += "-" + label;
    if (host.length() > 63) host = host.substring(0, 63);
    return host;
}

void Config::setRainDaily(float mm, int epochDay) {
    if (epochDay <= 0) return;
    // NVS asinmasi icin sadece anlamli degisimde yaz
    bool dayChanged = (epochDay != _rainDay);
    bool valueChanged = fabs(mm - _rainDaily) > 0.05f;
    _rainDaily = mm;
    _rainDay = epochDay;
    if (dayChanged || valueChanged) {
        _prefs.putFloat("raindaily", _rainDaily);
        _prefs.putInt("rainday", _rainDay);
    }
}

bool Config::persistAll() {
    bool ok = true;
    // putString basarisiz olursa 0 dondurur (NVS dolu / anahtar hatasi)
    ok &= _prefs.putString("ssid", _ssid) != 0;
    ok &= _prefs.putString("pass", _pass) != 0;
    ok &= _prefs.putString("api", _apiKey) != 0;
    ok &= _prefs.putString("station", _stationId) != 0;
    ok &= _prefs.putString("alias", _alias) != 0;
    ok &= _prefs.putFloat("lat", _lat) > 0;
    ok &= _prefs.putFloat("lon", _lon) > 0;
    ok &= _prefs.putInt("interval", _intervalMin) > 0;
    ok &= _prefs.putBool("deepsleep", _isDeepSleepEnabled) > 0;
    ok &= _prefs.putInt("txPower", _txPower) > 0;
    return ok;
}

bool Config::apply(const ConfigInput &in, String &err) {
    // --- Dogrulama ---
    if (in.ssid.length() == 0 || in.ssid.length() > CFG_SSID_MAX) {
        err = "SSID 1-" + String(CFG_SSID_MAX) + " karakter olmali";
        return false;
    }
    if (in.pass.length() > CFG_PASS_MAX) {
        err = "WiFi sifresi en fazla " + String(CFG_PASS_MAX) + " karakter";
        return false;
    }
    if (in.pass.length() > 0 && in.pass.length() < 8) {
        err = "WiFi sifresi 8 karakterden kisa olamaz (acik ag icin bos birakin)";
        return false;
    }
    if (in.apiKey.length() > CFG_APIKEY_MAX) {
        err = "API anahtari en fazla " + String(CFG_APIKEY_MAX) + " karakter";
        return false;
    }
    if (!validStationId(in.stationId)) {
        err = "Istasyon ID sadece harf, rakam, '-' ve '_' icerebilir (1-32)";
        return false;
    }
    if (in.alias.length() > CFG_ALIAS_MAX) {
        err = "Cihaz adi en fazla " + String(CFG_ALIAS_MAX) + " karakter";
        return false;
    }
    if (!validLat(in.lat) || !validLon(in.lon)) {
        err = "Gecersiz konum (lat -90..90, lon -180..180)";
        return false;
    }
    if (!validInterval(in.interval)) {
        err = "Olcum araligi " + String(CFG_INTERVAL_MIN) + "-" + String(CFG_INTERVAL_MAX) + " dakika olmali";
        return false;
    }
    if (!validTxPower(in.txPower)) {
        err = "Gecersiz WiFi guc degeri";
        return false;
    }

    // --- Uygula ---
    _ssid = in.ssid;
    _pass = in.pass;
    _apiKey = in.apiKey;
    _stationId = in.stationId;
    _alias = in.alias;
    _lat = in.lat;
    _lon = in.lon;
    _intervalMin = in.interval;
    _isDeepSleepEnabled = in.deepSleep;
    _txPower = in.txPower;

    if (!persistAll()) {
        err = "Ayarlar NVS'e yazilamadi";
        return false;
    }
    Serial.println("[Config] Ayarlar kaydedildi.");
    return true;
}

// --------------------------------------------------------------------------
// Seri arayuz: fiziksel USB erisimi guven siniri kabul edilir.
// GET_CONFIG  -> duz metin (web installer bunu kullanir)
// GET_CONFIG MASKED -> sirlar gizli
// --------------------------------------------------------------------------
void Config::checkSerialCommands() {
    static String rx;
    static unsigned long lastRx = 0;

    while (Serial.available()) {
        char c = Serial.read();
        lastRx = millis();
        if (c == '\n' || c == '\r') {
            if (rx.length() == 0) continue;
            String cmd = rx;
            rx = "";
            cmd.trim();
            if (cmd.length() == 0) continue;

            if (cmd.equals("GET_CONFIG") || cmd.equals("GET_CONFIG MASKED")) {
                bool masked = cmd.endsWith("MASKED");
                JsonDocument doc;
                doc["ssid"] = _ssid;
                doc["pass"] = masked ? (_pass.length() ? "********" : "") : _pass;
                doc["api"] = masked ? (_apiKey.length() ? "********" : "") : _apiKey;
                doc["station"] = _stationId;
                doc["alias"] = _alias;
                doc["lat"] = _lat;
                doc["lon"] = _lon;
                doc["interval"] = _intervalMin;
                doc["deepSleep"] = _isDeepSleepEnabled;
                doc["txPower"] = _txPower;
                doc["pin"] = masked ? "******" : _webPin;
                String response;
                serializeJson(doc, response);
                Serial.println(response);
            } else if (cmd.startsWith("SET_CONFIG ")) {
                if (cmd.length() > 768) {
                    Serial.println("VALIDATION_ERROR komut cok uzun");
                    continue;
                }
                JsonDocument doc;
                DeserializationError error = deserializeJson(doc, cmd.substring(11));
                if (error) {
                    Serial.println("JSON_ERROR");
                    continue;
                }
                ConfigInput in;
                in.ssid = _ssid;
                in.pass = _pass;
                in.apiKey = _apiKey;
                in.stationId = _stationId;
                in.alias = _alias;
                in.lat = _lat;
                in.lon = _lon;
                in.interval = _intervalMin;
                in.deepSleep = _isDeepSleepEnabled;
                in.txPower = _txPower;

                if (doc["ssid"].is<const char *>()) in.ssid = doc["ssid"].as<String>();
                if (doc["pass"].is<const char *>()) in.pass = doc["pass"].as<String>();
                if (doc["api"].is<const char *>()) in.apiKey = doc["api"].as<String>();
                if (doc["station"].is<const char *>()) in.stationId = doc["station"].as<String>();
                if (doc["alias"].is<const char *>()) in.alias = doc["alias"].as<String>();
                if (doc["lat"].is<float>() || doc["lat"].is<int>()) in.lat = doc["lat"].as<float>();
                if (doc["lon"].is<float>() || doc["lon"].is<int>()) in.lon = doc["lon"].as<float>();
                if (doc["interval"].is<int>()) in.interval = doc["interval"].as<int>();
                if (doc["deepSleep"].is<bool>()) in.deepSleep = doc["deepSleep"].as<bool>();
                if (doc["txPower"].is<int>()) in.txPower = doc["txPower"].as<int>();

                String err;
                if (!apply(in, err)) {
                    Serial.println("VALIDATION_ERROR " + err);
                    continue;
                }
                Serial.println("CONFIG_SAVED");
                delay(300);
                ESP.restart();
            } else if (cmd.equalsIgnoreCase("restart")) {
                Serial.println("Yeniden baslatiliyor...");
                delay(300);
                ESP.restart();
            } else if (cmd.equalsIgnoreCase("info")) {
                info();
            } else if (cmd.equalsIgnoreCase("pin")) {
                Serial.println("Web PIN: " + _webPin);
            } else if (cmd.equalsIgnoreCase("help")) {
                Serial.println("GET_CONFIG | GET_CONFIG MASKED | SET_CONFIG {json} | info | pin | restart");
            } else {
                Serial.println("Bilinmeyen komut. 'help' yazin.");
            }
        } else if (rx.length() < 768) {
            rx += c;
        }
    }

    // Yarim kalan satiri 5 sn sonra at
    if (rx.length() > 0 && millis() - lastRx > 5000) rx = "";
}

void Config::info() {
    Serial.println("--- Mevcut Ayarlar ---");
    Serial.println("SSID: " + _ssid);
    Serial.println("Sifre: " + String(_pass.length() ? "********" : "(yok)"));
    Serial.println("API Key: " + String(_apiKey.length() ? "********" : "(yok)"));
    Serial.println("Station: " + _stationId);
    Serial.println("Alias: " + _alias);
    Serial.println("Lat: " + String(_lat, 6));
    Serial.println("Lon: " + String(_lon, 6));
    Serial.println("Interval: " + String(_intervalMin) + " dk");
    Serial.println("Deep Sleep: " + String(_isDeepSleepEnabled ? "Aktif" : "Pasif"));
    Serial.println("TX Power: " + String(_txPower));
    Serial.println("Web PIN: " + _webPin);
}
