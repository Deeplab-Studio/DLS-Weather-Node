#include "DLSWeather.h"

DLSWeather::DLSWeather() {
    // Initialize default values
    _temp = 0.0;
    _humidity = 0.0;
    _pressure = 0.0;
    _windSpeed = 0.0;
    _windDir = 0.0;
    _rainRate = 0.0;
    _rainDaily = 0.0;
    _lastSentMinute = -1;
    _firstRun = true;
    _foundSensor = NONE;
    
    // Varsayılan ayarlar
    _ssid = "WIFI_SSID_GIRIN";
    _pass = "WIFI_SIFRE_GIRIN";
    _apiKey = "API_KEY";
    _stationId = "STATION_ID";
    _lat = 0.0;
    _lon = 0.0;
}

void DLSWeather::begin() {
    Serial.begin(115200);
    // while(!Serial); // Bekleme yapmayalım, boot gecikmesin
    
    Wire.begin(21, 22);

    loadPreferences();

    Serial.println("\n--- Yüklü Ayarlar ---");
    Serial.println("SSID: " + _ssid);
    Serial.println("API Key: " + _apiKey);
    Serial.println("Station ID: " + _stationId);
    Serial.println("Lat: " + String(_lat, 6));
    Serial.println("Lon: " + String(_lon, 6));
    Serial.println("---------------------");

    // Eğer SSID "WIFI_SSID_GIRIN" ise veya boşsa, yapılandırma bekle
    if (_ssid == "WIFI_SSID_GIRIN" || _ssid.isEmpty()) {
        Serial.println("\n!!! AYARLAR EKSİK !!!");
        Serial.println("Lütfen Serial üzerinden şu komutlarla ayarları girin:");
        Serial.println("ssid=WIFI_ADI");
        Serial.println("pass=WIFI_SIFRE");
        Serial.println("api=API_KEY");
        Serial.println("station=STATION_ID");
        Serial.println("Ayarlar girilene kadar bekleniyor...");

        Serial.println("station=STATION_ID");
        Serial.println("Ayarlar girilene kadar bekleniyor... (Bitince 'restart' komutu gönder)");

        while (true) {
             checkSerialCommands();
             delay(10);
        }
    }

    // Wi-Fi Başlatma
    connectWiFi();

    // NTP Başlatma
    _timeClient = new NTPClient(_ntpUDP, "pool.ntp.org", 0, 60000);
    _timeClient->begin();
    
    // Sensörleri Tara
    detectSensor();
}

void DLSWeather::loadPreferences() {
    _preferences.begin("dls-config", false);
    _ssid = _preferences.getString("ssid", _ssid);
    _pass = _preferences.getString("pass", _pass);
    _apiKey = _preferences.getString("api", _apiKey);
    _stationId = _preferences.getString("station", _stationId);
    _lat = _preferences.getFloat("lat", _lat);
    _lon = _preferences.getFloat("lon", _lon);
}

void DLSWeather::detectSensor() {
    Serial.println("\n--- Çoklu Sensör Tarama Başlatıldı ---");
    
    if (_bme680.begin(0x76)) {
        _foundSensor = BME680;
        Serial.println("SENSÖR: BME680 Tespit Edildi!");
        // BME680 Özel Ayarları
        _bme680.setTemperatureOversampling(BME680_OS_8X);
        _bme680.setHumidityOversampling(BME680_OS_2X);
        _bme680.setPressureOversampling(BME680_OS_4X);
        _bme680.setIIRFilterSize(BME680_FILTER_SIZE_3);
        _bme680.setGasHeater(320, 150); // 320°C, 150ms
    } else if (_bme.begin(0x76)) {
        _foundSensor = BME280;
        Serial.println("SENSÖR: BME280 Tespit Edildi!");
    } else if (_bmp.begin(0x76)) {
        _foundSensor = BMP280;
        Serial.println("SENSÖR: BMP280 Tespit Edildi!");
    } else {
        Serial.println("HATA: Hiçbir sensör bulunamadı! Kabloları kontrol et.");
    }
}

void DLSWeather::connectWiFi() {
    Serial.print("Wi-Fi Bağlanıyor: ");
    Serial.println(_ssid);
    WiFi.begin(_ssid.c_str(), _pass.c_str());

    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        delay(500);
        Serial.print(".");
        attempt++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWi-Fi Bağlandı!");
        Serial.print("IP Adresi: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nWi-Fi Başarısız. Loop içinde tekrar denenecek.");
    }
}

void DLSWeather::handle() {
    // 1. Wi-Fi Kontrolü
    if (WiFi.status() != WL_CONNECTED) {
        // Hemen spam yapmasın, 10 saniyede bir denesin
        static unsigned long lastReconnectAttempt = 0;
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 10000) {
             Serial.println("Wi-Fi Koptu! Tekrar bağlanılıyor...");
             WiFi.disconnect();
             WiFi.reconnect();
             lastReconnectAttempt = now;
        }
        // Döngüye devam etmeyelim, diğer işlemleri yapmasın (internet yoksa NTP vs çalışmaz)
        // Ancak sensör okumaya devam edebiliriz ama gönderemeyiz.
        checkSerialCommands(); // Hala komut alabilsin
        return; 
    }

    // 2. Serial Komutları Kontrol Et
    checkSerialCommands();

    // 3. Zamanı Güncelle
    _timeClient->update();
    int currentMinute = _timeClient->getMinutes();

    // 4. Sensör Verilerini Oku
    static unsigned long lastSensorRead = 0;
    if (millis() - lastSensorRead > 2000) { // 2 saniyede bir oku
        lastSensorRead = millis();
        // Sensör okuma mantığı
        switch (_foundSensor) {
            case BMP280:
                setEnvironment(_bmp.readTemperature(), 0, _bmp.readPressure() / 100.0F);
                break;
            case BME280:
                setEnvironment(_bme.readTemperature(), _bme.readHumidity(), _bme.readPressure() / 100.0F);
                break;
            case BME680:
                if (_bme680.performReading()) {
                     setEnvironment(_bme680.temperature, _bme680.humidity, _bme680.pressure / 100.0F);
                }
                break;
            default:
                break;
        }
    }

    // 5. Veri Gönderim Kontrolü (0 veya 30. dakika)
    if (_firstRun || ((currentMinute == 0 || currentMinute == 30) && currentMinute != _lastSentMinute)) {
        if (_firstRun) {
            Serial.println("İlk açılış gönderimi...");
        } else {
            Serial.println("Zamanı geldi (Dakika: " + String(currentMinute) + "), veriler gönderiliyor...");
        }

        // Simule edilmiş rüzgar ve yağmur verileri (Sensör yoksa)
        setWind(5.2, 180);
        setRain(0, 2.5);

        if (send()) {
            Serial.println("Veri başarıyla gönderildi!");
            _lastSentMinute = currentMinute;
            _firstRun = false;
        } else {
            Serial.println("Veri gönderme başarısız!");
        }
    }
}

void DLSWeather::checkSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd.startsWith("ssid=")) {
            String val = cmd.substring(5);
            _preferences.putString("ssid", val);
            _ssid = val; // RAM'i de güncelle
            Serial.println("SSID Kaydedildi: " + val);
        } else if (cmd.startsWith("pass=")) {
             String val = cmd.substring(5);
            _preferences.putString("pass", val);
            _pass = val; // RAM'i de güncelle
            Serial.println("Şifre Kaydedildi.");
        } else if (cmd.startsWith("api=")) {
             String val = cmd.substring(4);
            _preferences.putString("api", val);
             _apiKey = val; // RAM'i de güncelle
            Serial.println("API Key Kaydedildi.");
        } else if (cmd.startsWith("station=")) {
             String val = cmd.substring(8);
            _preferences.putString("station", val);
             _stationId = val; // RAM'i de güncelle
            Serial.println("Station ID Kaydedildi.");
        } else if (cmd.startsWith("lat=")) {
            String valStr = cmd.substring(4);
            valStr.replace(',', '.');
            float val = valStr.toFloat();
            _preferences.putFloat("lat", val);
            _lat = val;
            Serial.print("Lat Kaydedildi: ");
            Serial.println(val, 6);
        } else if (cmd.startsWith("lon=")) {
            String valStr = cmd.substring(4);
            valStr.replace(',', '.');
            float val = valStr.toFloat();
            _preferences.putFloat("lon", val);
            _lon = val;
            Serial.print("Lon Kaydedildi: ");
            Serial.println(val, 6);
        } else if (cmd.equalsIgnoreCase("restart")) {
            Serial.println("Yeniden başlatılıyor...");
            delay(1000);
            ESP.restart();
        } else if (cmd.equalsIgnoreCase("info")) {
            Serial.println("--- Ayarlar ---");
            Serial.println("SSID: " + _preferences.getString("ssid"));
            Serial.println("Station: " + _preferences.getString("station"));
        }
    }
}

void DLSWeather::setEnvironment(float temp, float humidity, float pressure) {
    _temp = temp;
    _humidity = humidity;
    _pressure = pressure;
    // Debug çıktısı
    // Serial.printf("Env: %.2fC, %.2f%%, %.2fhPa\n", temp, humidity, pressure);
}

void DLSWeather::setWind(float speed, float direction) {
    _windSpeed = speed;
    _windDir = direction;
}

void DLSWeather::setRain(float rate, float daily) {
    _rainRate = rate;
    _rainDaily = daily;
}

bool DLSWeather::send() {
    if (WiFi.status() != WL_CONNECTED) return false;

    _timeClient->update();
    unsigned long now = _timeClient->getEpochTime();

    if (now < 1704067200) { 
       Serial.println("Error: Time not yet synchronized (Epoch too old)");
       return false;
    }

    HTTPClient http;
    http.begin("https://wx-api.deeplabstudio.com/v1/ingest/weather");
    http.addHeader("Content-Type", "application/json");
    if (_apiKey.length() > 0) {
        http.addHeader("x-api-key", _apiKey);
    }

    JsonDocument doc;
    doc["stationId"] = _stationId;
    doc["timestamp"] = now;

    JsonObject location = doc["location"].to<JsonObject>();
    location["lat"] = _lat;
    location["lon"] = _lon;

    JsonObject environment = doc["environment"].to<JsonObject>();
    environment["temperature"] = _temp;
    environment["humidity"] = _humidity;
    environment["pressure"] = _pressure;

    JsonObject wind = doc["wind"].to<JsonObject>();
    wind["speed"] = _windSpeed;
    wind["direction"] = _windDir;

    JsonObject rain = doc["rain"].to<JsonObject>();
    rain["rate"] = _rainRate;
    rain["daily"] = _rainDaily;

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    Serial.println("Sending payload:");
    Serial.println(jsonOutput);

    int httpResponseCode = http.POST(jsonOutput);

    bool success = false;
    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        Serial.println(response);
        success = true;
    } else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();
    return success;
}
