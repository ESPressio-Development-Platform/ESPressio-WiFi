#include <Arduino.h>
#include <ESPressio_ESP32Persistence.hpp>
#include <ESPressio_ESP32WiFi.hpp>
#include <ESPressio_WiFiPersistence.hpp>

using namespace ESPressio;

Persistence::PreferencesStorage storage("espressio-wifi");
WiFi::ESP32WiFiPlatform platform;
WiFi::WiFiManager wifi(platform);
WiFi::KeyValueWiFiConfigurationStore configurationStore(storage, "configuration");

void setup() {
    Serial.begin(115200);

    if (storage.Initialize() != Persistence::StorageStatus::Success) {
        Serial.println("Persistence initialization failed");
        return;
    }

    wifi.SetConfigurationStore(&configurationStore);

    const auto loaded = wifi.LoadConfiguration(true);
    if (!loaded) {
        // First boot: create and persist a default AP configuration.
        WiFi::WiFiConfiguration configuration;
        configuration.AccessPoint.SSID = "ESPressio-Device";
        configuration.AccessPoint.Password = "change-this-password";

        if (wifi.Configure(configuration) != WiFi::WiFiStatus::Success) {
            Serial.println("WiFi configuration failed");
            return;
        }

        const auto saved = wifi.SaveConfiguration();
        Serial.println(saved ? "Default WiFi configuration persisted" : saved.Message.c_str());
    } else {
        Serial.println("Persisted WiFi configuration restored");
    }
}

void loop() {
    wifi.Poll();
    delay(10);
}
