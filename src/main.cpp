#include "DLSWeather.h"

DLSWeather* dls;

void setup() {
  // DLSWeather nesnesini oluştur ve başlat
  // Tüm sensör taramaları, Wi-Fi bağlantısı ve ayarlar kütüphane içinde yapılır.
  dls = new DLSWeather();
  dls->begin();
}

void loop() {
  // Sürekli döngü fonksiyonu
  // Wi-Fi kontrolü, sensör okuma, zamanlayıcılar ve serial komutları burada işlenir.
  dls->handle();
}