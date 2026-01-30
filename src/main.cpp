#include <Wire.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "DLSWeather.h"
#include "variant.h"
#include "Sensor/Sensor.h"
#include "NetworkManager/DLSNetwork.h"
#include "Display/Display.h"
#include "Config/Config.h"

// --- NESNELER ---
Config config;
DLSWeather* dls;
Sensor sensorManager;
DLSNetwork network;
Display display;
WebServer server(80); // Web Sunucusu

// --- GLOBAL VARIABLES (For API & Loop) ---
AirData latestAir;
LightData latestLight;
// Wind/Rain structs aren't defined in main scope yet, using placeholders for now
// If Sensor.h has them, we should use them, but Display uses internal structs. 
// We will rely on manual placeholders for API until Sensor logic is fully implemented.
float latestWindSpeed = -1.0;
float latestWindDir = -1.0;
float latestRainRate = -1.0;
float latestRainDaily = -1.0;

// --- DEGISKENLER ---
int lastSentMinute = -1;
bool firstRun = true;
unsigned long lastAttemptTime = 0;
bool pendingRetry = false;

// --- API handlers ---
void handleWeatherAPI() {
    // 512 bytes should be enough for this JSON
    JsonDocument doc;

    doc["status"] = true;
    
    // Air Data
    if (latestAir.valid) {
        if (latestAir.temperature != -999.0) doc["temperature"] = latestAir.temperature;
        else doc["temperature"] = nullptr;

        if (latestAir.humidity != -999.0) doc["humidity"] = latestAir.humidity;
        else doc["humidity"] = nullptr;

        if (latestAir.pressure != -999.0) doc["pressure"] = latestAir.pressure;
        else doc["pressure"] = nullptr;

        if (latestAir.gasResistance > 0 && latestAir.gasResistance != -999.0) 
            doc["air_quality"] = latestAir.gasResistance / 1000.0; // kOhm? User example said "300.4", likely raw or specific unit
        else doc["air_quality"] = nullptr;
    } else {
        doc["temperature"] = nullptr;
        doc["humidity"] = nullptr;
        doc["pressure"] = nullptr;
        doc["air_quality"] = nullptr;
    }

    // Light/UV
    if (latestLight.valid) {
        if (latestLight.uvIndex != -1.0) doc["uv_index"] = latestLight.uvIndex;
        else doc["uv_index"] = nullptr;
        // Lux not available in struct yet
    } else {
        doc["uv_index"] = nullptr;
    }

    // Wind/Rain (Placeholders)
    if (latestWindSpeed != -1.0) doc["wind_speed"] = latestWindSpeed; else doc["wind_speed"] = nullptr;
    if (latestWindDir != -1.0) doc["wind_dir"] = latestWindDir; else doc["wind_dir"] = nullptr;
    
    if (latestRainRate != -1.0) doc["rain_rate"] = latestRainRate; else doc["rain_rate"] = nullptr;
    if (latestRainDaily != -1.0) doc["rain_daily"] = latestRainDaily; else doc["rain_daily"] = nullptr;

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleNotFound() {
    String message = "{\"status\":false,\"error\":\"Not Found\"}";
    server.send(404, "application/json", message);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    // 1. Ayarlari Yukle
    config.begin();
    config.checkSerialCommands(); // Boot sirasinda komut yakalama sansi

    Serial.println("\n--- Yuklu Ayarlar ---");
    Serial.println("SSID: " + config.getSSID());
    Serial.println("Station ID: " + config.getStationID());
    Serial.println("Interval: " + String(config.getInterval()) + " dk");
    Serial.println("---------------------");

    // 2. I2C Baslat
    Wire.begin(I2C_SDA, I2C_SCL); 

    // 3. Ekrani Baslat
    display.begin(&Wire);
    display.printStartup(config.getSSID());

    // 4. Ayar Kontrolu
    if (config.getSSID() == "WIFI_SSID_GIRIN" || config.getSSID().isEmpty()) {
        display.showMessage("Ayar Eksik!");
        Serial.println("\n!!! AYARLAR EKSIK !!!");
        Serial.println("Lutfen Serial/Docs uzerinden ayarlari girin.");
        while (true) {
            config.checkSerialCommands();
            delay(10);
        }
    }

    // 5. Network Baslat
    network.begin(config.getSSID(), config.getPass(), LED_PIN);

    // 6. Sensor Baslat
    sensorManager.begin(&Wire);

    // 7. DLS Weather Kutuphanesi
    dls = new DLSWeather(
        config.getStationID(), 
        config.getAPIKey(), 
        config.getLat(), 
        config.getLon()
    );
    dls->begin();

    // 8. Web Server
    server.on("/api/weather", HTTP_GET, handleWeatherAPI);
    server.onNotFound(handleNotFound);
    server.begin();
    Serial.println("API Server Baslatildi.");
}

void loop() {
    network.update(); // Handles generic network tasks (e.g. WiFi KeepAlive if implemented)
    server.handleClient(); // Handle API stats
    config.checkSerialCommands();
    
    // --- DISPLAY UPDATE LOOP ---
    display.update();
    
    // Update Network Info on Display
    bool isConnected = network.isConnected();
    if (isConnected) {
        // WiFi localIP requires WiFi.h which is included in DLSNetwork.h
        display.setNetworkInfo(WiFi.localIP().toString(), config.getSSID(), "Online", true);
    } else {
        display.setNetworkInfo("0.0.0.0", config.getSSID(), "Offline", false);
    }

    int currentMinute = network.getMinutes();
    int interval = config.getInterval(); 
    if (interval <= 0) interval = 30; // Guvenlik

    // Veri Gonderimi Karari
    bool isScheduledTime = (currentMinute % interval == 0 && currentMinute != lastSentMinute);
    bool shouldAttempt = false;

    if (firstRun) {
        shouldAttempt = true;
    } else if (isScheduledTime) {
        shouldAttempt = true;
    } else if (pendingRetry && (millis() - lastAttemptTime > 60000)) {
        // Retry every 1 minute if failed (user requested 1 min for testing)
        shouldAttempt = true;
        Serial.println("\n[Retry] Re-attempting failed broadcast...");
    }

    if (shouldAttempt) {
        lastAttemptTime = millis();
        if (firstRun) {
            Serial.println("\n--- Ilk Acilis Verisi Hazirlaniyor ---");
        } else if (isScheduledTime) {
            Serial.println("\n--- Zamani Geldi, Veriler Okunuyor ---");
        }
        
        // --- 1. SENSOR OKUMA ---
        sensorManager.getAirData(latestAir);
        sensorManager.getLightData(latestLight);
        // Placeholder for future Wind/Rain
        // sensorManager.getWindData(latestWind);
        // sensorManager.getRainData(latestRain);

        // --- Display Data Update ---
        // Pass -999.0 if invalid, implementation handles printing "NaN"
        float gasRes = (latestAir.valid && latestAir.gasResistance > 0) ? latestAir.gasResistance : -999.0;
        display.setAirData(
            latestAir.valid ? latestAir.temperature : -999.0,
            latestAir.valid ? latestAir.humidity : -999.0,
            latestAir.valid ? latestAir.pressure : -999.0,
            gasRes
        );
        
        display.setLightData(
            latestLight.valid ? latestLight.uvIndex : -1.0,
            -1.0 // Lux placeholder
        );
        
        display.setWindData(-1.0, -1.0); // Speed, Dir
        display.setRainData(-1.0, -1.0); // Rate, Daily

        // --- Serial Monitor Log ---
        Serial.println("\n[Sensor Data]");
        if (latestAir.valid) {
            Serial.print("Temp: "); Serial.print(latestAir.temperature); Serial.println(" C");
            if (latestAir.humidity != -999.0) {
                Serial.print("Hum:  "); Serial.print(latestAir.humidity); Serial.println(" %");
            }
            Serial.print("Pres: "); Serial.print(latestAir.pressure); Serial.println(" hPa");
            if (latestAir.gasResistance > 0) {
                Serial.print("Gas:  "); Serial.print(latestAir.gasResistance); Serial.println(" KOhms");
            }
        } 

        if (latestLight.valid) {
            Serial.print("UV Idx: "); Serial.println(latestLight.uvIndex);
        }
        Serial.println("----------------");

        // --- 2. DLS Kutuphanesine Yazma (VALIDATION CHECK) ---
        if (latestAir.valid) {
            if (latestAir.temperature != -999.0) dls->temperature(latestAir.temperature);
            if (latestAir.humidity != -999.0)    dls->humidity(latestAir.humidity);
            if (latestAir.pressure != -999.0)    dls->pressure(latestAir.pressure);
            if (latestAir.gasResistance > 0 && latestAir.gasResistance != -999.0) 
                dls->airQuality(latestAir.gasResistance);
        }

        if (latestLight.valid) {
             if (latestLight.uvIndex != -1.0) dls->uvIndex(latestLight.uvIndex);
        }

        // --- 3. Gonderim (Sadece bagliysa) ---
        if (isConnected) {
            display.setStatus("Sending...");
            display.update(); // Force update to show sending
            
            if (dls->send(network.getEpochTime())) {
                Serial.println("Basariyla gonderildi.");
                display.setStatus("Success!");
                lastSentMinute = currentMinute;
                firstRun = false; 
                pendingRetry = false;
            } else {
                int errCode = dls->getLastCode();
                Serial.print("[Retry] Gonderme hatasi! Kod: "); Serial.println(errCode);
                
                String errStr;
                if (errCode == -1) errStr = "WiFi Err";
                else if (errCode > 0) errStr = "HTTP " + String(errCode);
                else errStr = "Conn Err";
                
                display.setStatus(errStr, true);
                pendingRetry = true;
                firstRun = false;
            }
        } else {
            Serial.println("[Retry] WiFi bagli degil! 1 dk sonra tekrar denenecek.");
            display.setStatus("No WiFi", true);
            pendingRetry = true;
            firstRun = false;
        }
    } else {
        // If we are NOT sending data, we should still update sensors periodically 
        // to keep the API and Display fresh! 
        // Otherwise API returns old data until next upload cycle (e.g. 30 mins!)
        
        // Let's read sensors every 5 seconds or so, separate from upload logic?
        // For simplicity and RAM safety, let's just read them every loop iteration for now?
        // NO, reading I2C too fast is bad. Every 2-5 seconds is good.
        
        static unsigned long lastSensorRead = 0;
        if (millis() - lastSensorRead > 2000) {
            lastSensorRead = millis();
            sensorManager.getAirData(latestAir);
            sensorManager.getLightData(latestLight);
            
            // Update Display Immediate (Optional, display.update() handles paging, but data setters need calling)
            // But we already set data inside the upload block. 
            // We should move set*Data calls OUTSIDE the upload block to keep display fresh.
            
            float gasRes = (latestAir.valid && latestAir.gasResistance > 0) ? latestAir.gasResistance : -999.0;
            display.setAirData(
                latestAir.valid ? latestAir.temperature : -999.0,
                latestAir.valid ? latestAir.humidity : -999.0,
                latestAir.valid ? latestAir.pressure : -999.0,
                gasRes
            );
            
            display.setLightData(
                latestLight.valid ? latestLight.uvIndex : -1.0,
                -1.0 
            );
            // Wind/Rain placeholdes
            display.setWindData(-1.0, -1.0);
            display.setRainData(-1.0, -1.0); 
        }
    }

    delay(10); // Short delay for stability
}
