#include <Arduino.h>
#include <ESPressio_ESP32Persistence.hpp>
#include <ESPressio_ESP32WiFi.hpp>
#include <ESPressio_WiFiPersistence.hpp>
#include <ESPressio_WiFiWorker.hpp>

using namespace ESPressio;

Persistence::PreferencesStorage storage("espressio-wifi");
WiFi::ESP32WiFiPlatform platform;
WiFi::WiFiManager wifi(platform);
WiFi::WiFiWorker wifiWorker(wifi);
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
        // First boot: create and persist remembered Client networks.
        WiFi::WiFiConfiguration configuration;
        configuration.Mode = WiFi::WiFiMode::Client;
        configuration.Client.Enabled = true;

        WiFi::ClientNetworkProfile primary;
        primary.SSID = "Primary-Network";
        primary.Password = "replace-primary-password";
        primary.Priority = 300;

        WiFi::ClientNetworkProfile fallback;
        fallback.SSID = "Fallback-Network";
        fallback.Password = "replace-fallback-password";
        fallback.Priority = 100;

        configuration.Client.Networks = { primary, fallback };

        if (wifi.Configure(configuration) != WiFi::WiFiStatus::Success) {
            Serial.println("WiFi configuration failed");
            return;
        }

        const auto saved = wifi.SaveConfiguration();
        Serial.println(saved ? "Default WiFi configuration persisted" : saved.Message.c_str());
    } else {
        Serial.println("Persisted WiFi configuration restored");
    }

    if (wifiWorker.Initialize() != Threads::ThreadInitializationStatus::Success ||
        wifiWorker.Start() != Threads::ThreadInitializationStatus::Success) {
        Serial.println("WiFi worker failed to start");
    }
}

void loop() {
    // WiFiWorker services scans, selection, connection and failover.
    delay(1000);
}
