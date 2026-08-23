#include <Arduino.h>
#include <ESPressio_WiFi.hpp>
#include <ESPressio_WiFiWorker.hpp>
#include <ESPressio_ESP32WiFi.hpp>

using namespace ESPressio::WiFi;

ESP32WiFiPlatform platform;
WiFiManager wifi(platform);
WiFiWorker wifiWorker(wifi);

void setup() {
    Serial.begin(115200);

    WiFiConfiguration config;
    config.AccessPoint.SSID = "ESPressio-Example";
    config.AccessPoint.Password = "replace-this-password";
    config.AccessPoint.Channel = 6;

    wifi.OnAccessPointStateChanged([](const auto&, const auto& after) {
        Serial.printf(
            "AP state=%u stations=%u\n",
            static_cast<unsigned>(after.State),
            static_cast<unsigned>(after.ConnectedStations)
        );
    });

    if (wifi.Configure(config) != WiFiStatus::Success) {
        Serial.println("WiFi configuration failed");
        return;
    }

    if (wifiWorker.Initialize() != ESPressio::Threads::ThreadInitializationStatus::Success ||
        wifiWorker.Start() != ESPressio::Threads::ThreadInitializationStatus::Success) {
        Serial.println("WiFi worker failed to start");
    }
}

void loop() {
    // WiFiWorker services ESPressio WiFi on PrecisionThread.
    delay(1000);
}
