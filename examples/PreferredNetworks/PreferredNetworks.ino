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
    config.Mode = WiFiMode::Client;
    config.Client.Enabled = true;

    ClientNetworkProfile primary;
    primary.SSID = "Primary-Network";
    primary.Password = "replace-primary-password";
    primary.Priority = 300;

    ClientNetworkProfile secondary;
    secondary.SSID = "Secondary-Network";
    secondary.Password = "replace-secondary-password";
    secondary.Priority = 200;

    ClientNetworkProfile hotspot;
    hotspot.SSID = "Phone-Hotspot";
    hotspot.Password = "replace-hotspot-password";
    hotspot.Priority = 100;

    config.Client.Networks = { primary, secondary, hotspot };

    wifi.OnClientNetworkSelected([](const ClientNetworkCandidate& selected) {
        Serial.printf(
            "Selected SSID=%s priority=%u RSSI=%d channel=%u\n",
            selected.SSID.c_str(),
            static_cast<unsigned>(selected.Priority),
            static_cast<int>(selected.RSSI),
            static_cast<unsigned>(selected.Channel)
        );
    });

    wifi.OnClientNoKnownNetworkAvailable([]() {
        Serial.println("No remembered network is currently visible");
    });

    wifi.OnClientStateChanged([](const ClientRuntimeState&, const ClientRuntimeState& after) {
        Serial.printf("Client state=%u SSID=%s\n",
            static_cast<unsigned>(after.State), after.SSID.c_str());
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
    // Scan, selection, connection and failover are serviced by WiFiWorker.
    delay(1000);
}
