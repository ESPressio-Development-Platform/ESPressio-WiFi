# ESPressio WiFi

Platform-neutral WiFi lifecycle, configuration and diagnostics for ESP32 applications.

## Version — 0.1.0

0.1.0 is the first real ESPressio WiFi API. The old manifest-only `1.0.0` value was never a released implementation and is intentionally superseded.

## The shortest possible setup

WiFi defaults to Access Point mode. Supply the credentials, apply the configuration, then poll the manager from your application loop or worker thread:

```cpp
#include <ESPressio_WiFi.hpp>
#include <ESPressio_ESP32WiFi.hpp>

ESPressio::WiFi::ESP32WiFiPlatform platform;
ESPressio::WiFi::WiFiManager wifi(platform);

void setup() {
    ESPressio::WiFi::WiFiConfiguration config;
    config.AccessPoint.SSID = "my-device";
    config.AccessPoint.Password = "change-me-now";
    wifi.Configure(config);
}

void loop() {
    wifi.Poll();
}
```

The public manager and configuration APIs expose only ESPressio types. `IPAddress`, `wifi_event_t`, Arduino status values and ESP-IDF structures belong inside the ESP32 adapter.

## Client mode

```cpp
WiFiConfiguration config;
config.Mode = WiFiMode::Client;
config.Client.Enabled = true;
config.Client.SSID = "office";
config.Client.Password = "secret";
wifi.Configure(config);
wifi.ConnectClient();
```

`ClientRuntimeState` independently reports `Idle`, `Connecting`, `Connected`, `Reconnecting` and `Failed`. AP state remains independently visible, which makes `AccessPointClient` mode unambiguous.

## AP + Client

```cpp
config.Mode = WiFiMode::AccessPointClient;
config.AccessPoint.Enabled = true;
config.Client.Enabled = true;
wifi.Configure(config);
```

The composite `WiFiRuntimeState` contains both state machines and a monotonically increasing revision.

## Asynchronous scanning

```cpp
wifi.OnScanCompleted([](const std::vector<ScanResult>& networks) {
    for (const auto& network : networks) {
        // SSID, ESPressio MacAddress, RSSI, channel, security, hidden
    }
});

wifi.Scan();
```

Scanning never exposes Arduino scan-result types.

## DHCP and static addressing

Client addressing defaults to DHCP. Set `Client.Addressing = AddressMode::Static` and populate `Client.StaticNetwork` for a static address, gateway, mask and DNS servers.

The AP owns an independent network configuration and DHCP-server switch. The default AP network is `192.168.4.1/24`.

## Persisting configuration

Configuration is always an ESPressio Serializable model. Persistence is optional:

```cpp
#include <ESPressio_WiFiPersistence.hpp>

WiFiConfigurationStore::Save(storage, "/wifi.espb", config);
WiFiConfigurationStore::Load(storage, "/wifi.espb", config);
```

The same helper accepts `IKeyValueStorage` for Preferences/NVS-style storage.

### Protect credentials at rest

WiFi passwords are marked `Sensitive()` for redacted diagnostic serialization, but redaction is **not encryption**. For persisted credentials, protected persistence is strongly recommended:

```cpp
#include <ESPressio_WiFiPersistenceSecurity.hpp>

Serializable::SerializationProtectionConfig protection;
protection.Protector = &protector;
protection.Context = "wifi-configuration";

ProtectedWiFiConfigurationStore::Save(
    storage, "/wifi.esdp", config, protection
);
```

This delegates authenticated protection to Serializable 0.11.x / Security 0.4.x through Persistence 0.3.x. WiFi never chooses a cipher or owns cryptographic key policy.

## State callbacks

```cpp
wifi.OnStateChanged([](const WiFiRuntimeState& before,
                       const WiFiRuntimeState& after) {
    // React synchronously to logical state changes.
});
```

Optional Observable/Event adapters can layer on this state model without changing the core platform contract.

## Architecture

```text
application
    |
WiFiManager + WiFiConfiguration (Serializable)
    |
IWiFiPlatform                 optional integrations
    |                         - - > Persistence 0.3.x
ESP32WiFiPlatform             - - > Security via protected Persistence
                              - - > Observable / Event / Command
```

Web servers, HTTP, WebSockets, captive portals and browser UI are intentionally outside this library.

## Dependency policy

Serializable 0.11.x is the only foundational dependency because WiFi configuration is explicitly Serializable. Persistence, Security, Observable, Event and Command are opt-in integration surfaces.

See `ESPRESSIO_DEPENDENCY_CHART.md` for the complete dependency position.
