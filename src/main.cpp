#include <Wire.h>
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

// --- DEGISKENLER ---
int lastSentMinute = -1;
bool firstRun = true;

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
}

void loop() {
    network.update();
    config.checkSerialCommands();
    
    if (!network.isConnected()) {
        display.showMessage("Wi-Fi Kayip...");
        return;
    }

    int currentMinute = network.getMinutes();
    int interval = config.getInterval(); 
    if (interval <= 0) interval = 30; // Guvenlik

    // Veri Gonderimi
    // Mantik: Dakika, intervalin kati oldugunda VE bu dilimde henuz gondermediysek.
    // Ornek: interval=15 -> 0, 15, 30, 45...
    bool isTimeToSend = (currentMinute % interval == 0);

    if (firstRun || (isTimeToSend && currentMinute != lastSentMinute)) {
        if (firstRun) {
            Serial.println("\n--- İlk Acilis Verisi Hazirlaniyor ---");
            display.showMessage("Veri Okunuyor...");
        } else {
            Serial.println("\n--- Zamanı Geldi, Veriler Okunuyor ---");
        }
        
        // --- 1. SENSOR OKUMA ---
        AirData airValues;
        sensorManager.getAirData(airValues);

        LightData lightValues;
        sensorManager.getLightData(lightValues);

        // --- Serial Monitor Log ---
        Serial.println("\n[Sensor Data]");
        if (airValues.valid) {
            Serial.print("Temp: "); Serial.print(airValues.temperature); Serial.println(" C");
            Serial.print("Hum:  "); Serial.print(airValues.humidity); Serial.println(" %");
            Serial.print("Pres: "); Serial.print(airValues.pressure); Serial.println(" hPa");
            if (airValues.gasResistance > 0) {
                Serial.print("Gas:  "); Serial.print(airValues.gasResistance); Serial.println(" KOhms");
            }
        } else {
            Serial.println("Hava sensoru verisi gecersiz veya yok.");
        }

        if (lightValues.valid) {
            Serial.print("UV Idx: "); Serial.println(lightValues.uvIndex);
            Serial.print("UVA:    "); Serial.println(lightValues.uva);
            Serial.print("UVB:    "); Serial.println(lightValues.uvb);
        }
        Serial.println("----------------");

        // --- 2. DLS Kutuphanesine Yazma ---
        if (airValues.valid) {
            dls->temperature(airValues.temperature);
            dls->humidity(airValues.humidity);
            dls->pressure(airValues.pressure);
            if (airValues.gasResistance > 0) dls->airQuality(airValues.gasResistance);
        }

        // --- 3. Gonderim ---
        String statusMsg = "Wait";
        if (dls->send(network.getEpochTime())) {
            Serial.println("Basariyla gonderildi.");
            statusMsg = "Sent OK";
            lastSentMinute = currentMinute;
            firstRun = false; 
        } else {
            Serial.println("Gonderme hatasi!");
            statusMsg = "Err";
            
            // Basarisiz olursa lastSentMinute guncelleme, belki 1dk sonra tekrar dener?
            // Veya bu cycle gecti artik diyebiliriz.
            // Simdilik firstRun ise kapat, degilse cycle'i kapat.
             lastSentMinute = currentMinute; 
             firstRun = false; 
        }
        
        // --- 4. Ekrana Yazma ---
        float uvVal = lightValues.valid ? lightValues.uvIndex : -1.0;
        display.printData(
            airValues.valid ? airValues.temperature : 0.0, 
            airValues.valid ? airValues.humidity : 0.0, 
            airValues.valid ? airValues.pressure : 0.0, 
            uvVal,
            statusMsg
        );
    }

    delay(1000);
}
