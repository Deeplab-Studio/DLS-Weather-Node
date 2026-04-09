#include <Wire.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "DLSWeather.h"
#include "variant.h"
#include "Sensor/Sensor.h"
#include "NetworkManager/DLSNetwork.h"
#include "Display/Display.h"
#include "Config/Config.h"
#include <esp_sleep.h>
#include <esp_system.h>

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
WindData latestWind;
RainData latestRain;

// --- DEGISKENLER ---
int lastSentMinute = -1;
bool firstRun = true;
unsigned long lastAttemptTime = 0;
bool pendingRetry = false;
bool isFromSleep = false;
unsigned long bootTime = 0;

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

    // Wind/Rain
    if (latestWind.valid) {
        if (latestWind.speed != -1.0) doc["wind_speed"] = latestWind.speed; else doc["wind_speed"] = nullptr;
        if (latestWind.direction != -1.0) doc["wind_dir"] = latestWind.direction; else doc["wind_dir"] = nullptr;
    } else {
        doc["wind_speed"] = nullptr;
        doc["wind_dir"] = nullptr;
    }
    
    if (latestRain.valid) {
        if (latestRain.rate != -1.0) doc["rain_rate"] = latestRain.rate; else doc["rain_rate"] = nullptr;
        if (latestRain.daily != -1.0) doc["rain_daily"] = latestRain.daily; else doc["rain_daily"] = nullptr;
    } else {
        doc["rain_rate"] = nullptr;
        doc["rain_daily"] = nullptr;
    }

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleNotFound() {
    String message = "{\"status\":false,\"error\":\"Not Found\"}";
    server.send(404, "application/json", message);
}

// --- Web UI Handlers ---
void handleWebRoot() {
    String html = "<!DOCTYPE html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width, initial-scale=1'><title>DLS Weather Config</title>";
    html += "<style>body{font-family:sans-serif;margin:20px;background:#f0f2f5;} .container{max-width:500px;margin:auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 5px rgba(0,0,0,0.1);} h2{text-align:center;color:#333;} label{display:block;margin-top:10px;font-weight:bold;} input,select{width:100%;padding:10px;margin-top:5px;border:1px solid #ddd;border-radius:5px;box-sizing:border-box;} button{width:100%;background:#2563eb;color:white;padding:12px;border:none;border-radius:5px;margin-top:20px;cursor:pointer;font-size:16px;} button:hover{background:#1d4ed8;}</style>";
    html += "</head><body><div class='container'><h2>Device Configuration</h2>";
    html += "<form action='/save' method='POST'>";
    
    html += "<label>WiFi SSID</label><input type='text' name='ssid' value='" + config.getSSID() + "'>";
    html += "<label>WiFi Password</label><input type='password' name='pass' value='" + config.getPass() + "'>";
    
    html += "<label>API Key</label><input type='text' name='api' value='" + config.getAPIKey() + "'>";
    html += "<label>Station ID</label><input type='text' name='station' value='" + config.getStationID() + "'>";
    html += "<label>Alias (Optional)</label><input type='text' name='alias' value='" + config.getAlias() + "'>";
    
    html += "<div style='display:flex;gap:10px;'>";
    html += "<div style='flex:1;'><label>Lat</label><input type='text' name='lat' value='" + String(config.getLat(), 5) + "'></div>";
    html += "<div style='flex:1;'><label>Lon</label><input type='text' name='lon' value='" + String(config.getLon(), 5) + "'></div>";
    html += "</div>";
    
    html += "<label>Interval (Min)</label><select name='interval'>";
    int iv = config.getInterval();
    html += "<option value='15'" + String(iv==15?" selected":"") + ">15</option>";
    html += "<option value='20'" + String(iv==20?" selected":"") + ">20</option>";
    html += "<option value='25'" + String(iv==25?" selected":"") + ">25</option>";
    html += "<option value='30'" + String(iv==30?" selected":"") + ">30</option>";
    html += "</select>";
    
    html += "<label>WiFi Power</label><select name='txPower'>";
    int tx = config.getTxPower();
    html += "<option value='78'" + String(tx==78?" selected":"") + ">19.5 dBm (Max)</option>";
    html += "<option value='68'" + String(tx==68?" selected":"") + ">17 dBm</option>";
    html += "<option value='60'" + String(tx==60?" selected":"") + ">15 dBm</option>";
    html += "<option value='52'" + String(tx==52?" selected":"") + ">13 dBm</option>";
    html += "<option value='44'" + String(tx==44?" selected":"") + ">11 dBm</option>";
    html += "<option value='34'" + String(tx==34?" selected":"") + ">8.5 dBm (Default)</option>";
    html += "<option value='8'" + String(tx==8?" selected":"") + ">2 dBm (Low)</option>";
    html += "</select>";
    
    html += "<label>Deep Sleep</label><select name='deepSleep'>";
    bool ds = config.isDeepSleepEnabled();
    html += "<option value='0'" + String(!ds?" selected":"") + ">OFF (Indoor/USB)</option>";
    html += "<option value='1'" + String(ds?" selected":"") + ">ON (Battery/Roof)</option>";
    html += "</select>";
    
    html += "<button type='submit'>Save & Restart</button>";
    html += "</form></div></body></html>";
    
    server.send(200, "text/html", html);
}

void handleWebSave() {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }
    
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    String api = server.arg("api");
    String station = server.arg("station");
    String alias = server.arg("alias");
    float lat = server.arg("lat").toFloat();
    float lon = server.arg("lon").toFloat();
    int interval = server.arg("interval").toInt();
    int txPower = server.arg("txPower").toInt();
    bool deepSleep = (server.arg("deepSleep") == "1");
    
    config.saveConfig(ssid, pass, api, station, alias, lat, lon, interval, deepSleep, txPower);
    
    String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='30;url=/'><meta name='viewport' content='width=device-width, initial-scale=1'></head>";
    html += "<body style='font-family:sans-serif;text-align:center;padding:50px;'>";
    html += "<h2>Settings Saved!</h2><p>Device is restarting...</p><p>Please reconnect to: <b>" + ssid + "</b></p></body></html>";
    
    server.send(200, "text/html", html); // Send response before restart
    delay(1000); // Give time to flush
    ESP.restart();
}

void setup() {
    // 0. SENSOR POWER ON (MOSFET)
    pinMode(SENSOR_PWR_PIN, OUTPUT);
    digitalWrite(SENSOR_PWR_PIN, HIGH);
    
    Serial.begin(115200);
    delay(3000); // Give sensors and serial time to stabilize
    // 1. Ayarlari Yukle
    config.begin();

    // Check reset reason
    isFromSleep = (esp_reset_reason() == ESP_RST_DEEPSLEEP);
    bootTime = millis();

    config.checkSerialCommands(); // Boot sirasinda komut yakalama sansi

    Serial.println("\n--- Yuklu Ayarlar ---");
    Serial.println("SSID: " + config.getSSID());
    Serial.println("Station ID: " + config.getStationID());
    if (!config.getAlias().isEmpty()) Serial.println("Alias: " + config.getAlias());
    Serial.println("Interval: " + String(config.getInterval()) + " dk");
    Serial.println("---------------------");

    // 2. I2C Baslat
    Wire.begin(I2C_SDA, I2C_SCL); 

    // 3. Ekrani Baslat
    display.begin(&Wire);
    
    // Show Startup Screen with SSID and Alias
    display.printStartup(config.getSSID(), config.getAlias());

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
    // Set Hostname FIRST for router recognition
    String hostname = "dls-weather";
    if (!config.getStationID().isEmpty() && config.getStationID() != "ST-XXXXX") {
        hostname += "-" + config.getStationID();
    }
    WiFi.setHostname(hostname.c_str());

    // Apply configured TX Config
    // config.getTxPower() returns int, we need to cast to wifi_power_t
    network.setTxPower((wifi_power_t)config.getTxPower());
    network.begin(config.getSSID(), config.getPass(), LED_PIN);
    
    // 5b. mDNS Baslat
    network.startMDNS(hostname.c_str());

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
    server.on("/", HTTP_GET, handleWebRoot);
    server.on("/save", HTTP_POST, handleWebSave);
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
        // Deep Sleep aktifse ve cihaz yeni açılmışsa (uykudan uyanmamışsa),
        // ilk veriyi göndermeden önce 5 dakika bekle. Bu yapılandırma penceresidir.
        if (config.isDeepSleepEnabled() && !isFromSleep) {
            if (millis() - bootTime > 300000) {
                shouldAttempt = true;
                Serial.println("\n[DeepSleep] Startup delay finished. Triggering first broadcast.");
            } else {
                static unsigned long lastMsg = 0;
                if (millis() - lastMsg > 60000) {
                    lastMsg = millis();
                    Serial.print("[DeepSleep] Waiting for config window... ");
                    Serial.print((300000 - (millis() - bootTime)) / 1000);
                    Serial.println("s remaining.");
                }
                // Ayar gelme ihtimaline karsi Web Server'i calistir
                server.handleClient();
            }
        } else if (isFromSleep) {
            // Uykudan uyanmissak hemen gonder (zaten uyku suresi doldu)
            shouldAttempt = true;
            Serial.println("\n[DeepSleep] Wake-up detected. Sending data immediately.");
        } else {
            // Normal modda hemen basla
            shouldAttempt = true;
        }
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
        sensorManager.getWindData(latestWind);
        sensorManager.getRainData(latestRain);

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
        
        display.setWindData(
            latestWind.valid ? latestWind.speed : -1.0,
            latestWind.valid ? latestWind.direction : -1.0
        );
        display.setRainData(
            latestRain.valid ? latestRain.rate : -1.0,
            latestRain.valid ? latestRain.daily : -1.0
        );

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
        if (latestWind.valid) {
            Serial.print("Wind Spd: "); Serial.print(latestWind.speed); Serial.println(" m/s");
            Serial.print("Wind Dir: "); Serial.print(latestWind.direction); Serial.println(" deg");
        }
        if (latestRain.valid) {
            Serial.print("Rain Rate: "); Serial.print(latestRain.rate); Serial.println(" mm/h");
            Serial.print("Rain Daily: "); Serial.print(latestRain.daily); Serial.println(" mm");
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

        if (latestWind.valid) {
             if (latestWind.speed != -1.0) dls->windSpeed(latestWind.speed);
             if (latestWind.direction != -1.0) dls->windDirection(latestWind.direction);
        }

        if (latestRain.valid) {
             if (latestRain.rate != -1.0) dls->rainRate(latestRain.rate);
             if (latestRain.daily != -1.0) dls->rainDaily(latestRain.daily);
        }

        // --- Battery Read & Send ---
        #ifdef ADC_PIN
            // Use calibrated millivolts reading
            uint32_t calib_mV = analogReadMilliVolts(ADC_PIN);
            float voltage = (calib_mV / 1000.0) * ADC_MULTIPLIER;
            
            int pct = map(voltage * 100, 300, 420, 0, 100);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            
            // Send to Display (so it shows up during send)
            display.setBatteryData(voltage, pct);

            // Send to API
            dls->battery(pct);
            dls->voltage(voltage);
            
            Serial.printf("[Battery] Raw mV: %d | Calc: %.2fV (%d%%)\n", calib_mV, voltage, pct);
        #endif

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

                // --- DEEP SLEEP CHECK ---
                if (config.isDeepSleepEnabled()) {
                    int interval = config.getInterval();
                    if (interval <= 0) interval = 30; // Safety

                    // Full interval sleep as requested (prevent drift alignment errors)
                    long sleepSeconds = (long)interval * 60;

                    Serial.print("\n[DeepSleep] Entering sleep for ");
                    Serial.print(sleepSeconds);
                    Serial.println(" seconds... ");
                    
                    display.setStatus("Sleeping...");
                    display.update();
                    delay(2000); // Give time for display/serial
                    
                    display.off(); // Clear and turn off screen
                    
                    // SENSOR POWER OFF (MOSFET)
                    digitalWrite(SENSOR_PWR_PIN, LOW);

                    // ESP32 deep sleep takes microseconds
                    esp_sleep_enable_timer_wakeup((uint64_t)sleepSeconds * 1000000);
                    esp_deep_sleep_start();
                }
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
            
            #ifdef DEBUG_MODE
            if (DEBUG_MODE) {
                 Serial.println("[DEBUG] WiFi failed but continuing loop via DEBUG_MODE.");
            }
            #endif
        }
    } else {
        // If we are NOT sending data, we should still update sensors periodically 
        // to keep the API and Display fresh! 
        // Otherwise API returns old data until next upload cycle (e.g. 30 mins!)
        
        // Let's read sensors every 5 seconds or so, separate from upload logic?
        // For simplicity and RAM safety, let's just read them every loop iteration for now?
        // NO, reading I2C too fast is bad. Every 2-5 seconds is good.
        
        static unsigned long lastSensorRead = 0;
        if (millis() - lastSensorRead > 1000) {
            lastSensorRead = millis();
            sensorManager.getAirData(latestAir);
            sensorManager.getLightData(latestLight);
            
            // --- Battery Read ---
            #ifdef ADC_PIN
                uint32_t calib_mV = analogReadMilliVolts(ADC_PIN);
                float voltage = (calib_mV / 1000.0) * ADC_MULTIPLIER;
                
                // Simple Percentage (3.0V - 4.2V)
                int pct = map(voltage * 100, 300, 420, 0, 100);
                if (pct < 0) pct = 0;
                if (pct > 100) pct = 100;

                display.setBatteryData(voltage, pct);
                
                // Log Battery periodically
                static unsigned long lastBatLog = 0;
                if (millis() - lastBatLog > 10000) {
                     lastBatLog = millis();
                     Serial.printf("[Battery] %.2fV (%d%%)\n", voltage, pct);
                }
            #endif

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
            sensorManager.getWindData(latestWind);
            sensorManager.getRainData(latestRain);
            
            display.setWindData(
                latestWind.valid ? latestWind.speed : -1.0,
                latestWind.valid ? latestWind.direction : -1.0
            );
            display.setRainData(
                latestRain.valid ? latestRain.rate : -1.0,
                latestRain.valid ? latestRain.daily : -1.0
            );
            
            // --- LOG ALL DATA EVERY SECOND FOR TESTING ---
            Serial.println("\n--- 1s Test Log ---");
            if (latestAir.valid) {
                Serial.printf("Air: %.1fC | %.1f%% | %.1fhPa\n", latestAir.temperature, latestAir.humidity, latestAir.pressure);
            }
            if (latestWind.valid) {
                Serial.printf("Wind: %.2f m/s | %.1f deg\n", latestWind.speed, latestWind.direction);
            }
            if (latestRain.valid) {
                Serial.printf("Rain: %.2f mm/h | %.2f mm today\n", latestRain.rate, latestRain.daily);
            }
            Serial.println("-------------------");

            // Handle DEBUG MODE (Skip WiFi check for sending if Debug is ON?)
            // Actually, usually Debug mode just means "Don't crash if no WiFi"
            // But here we want to see sensor values on screen even if offline.
            // Which is already happening because this block runs every 1s!
            // So we just need to ensure we don't block invalid operations.
        }
    }

    delay(10); // Short delay for stability
}
