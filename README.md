# ESPressio WiFi

Platform-neutral, event-driven WiFi lifecycle and configuration for ESP32 applications.

## Version — 0.1.0

0.1.0 is the first functional release. The historical manifest-only `1.0.0` value never represented a released implementation and creates no compatibility obligation.

## What ESPressio WiFi owns

- Access Point, Client and AP+Client operating modes.
- Independent AP and Client runtime state machines.
- ESPressio-owned IPv4/MAC/network/security/scan types; no Arduino/ESP-IDF types in public APIs.
- Always-Serializable WiFi configuration.
- DHCP/static client addressing and AP DHCP-server configuration data.
- asynchronous scanning.
- direct callbacks and ESPressio Observable notifications.
- an explicit ESP32 implementation behind `IWiFiPlatform`.
- optional Persistence, protected Persistence/Security, Event and Command integrations.

HTTP, WebSocket, browser UI and other Web concerns deliberately belong elsewhere.

## Minimal Access Point

WiFi defaults to Access Point mode:

```cpp
#include <ESPressio_WiFi.hpp>
#include <ESPressio_ESP32WiFi.hpp>

using namespace ESPressio::WiFi;

ESP32WiFiPlatform platform;
WiFiManager wifi(platform);

void setup() {
    WiFiConfiguration config;
    config.AccessPoint.SSID = "ESPressio-Device";
    config.AccessPoint.Password = "change-me";
    wifi.Configure(config);
}

void loop() {
    wifi.Poll();
}
```

## Client and AP+Client modes

```cpp
WiFiConfiguration config;
config.Mode = WiFiMode::Client;
config.Client.Enabled = true;
config.Client.SSID = "Studio";
config.Client.Password = "secret";
wifi.Configure(config);
wifi.ConnectClient();
```

For simultaneous AP and Client operation:

```cpp
config.Mode = WiFiMode::AccessPointClient;
config.AccessPoint.Enabled = true;
config.Client.Enabled = true;
wifi.Configure(config);
```

`WiFiRuntimeState` then reports both independently, for example `AccessPoint.State == Active` while `Client.State == Connecting`.

## DHCP and static addressing

Client addressing defaults to DHCP. Static addressing is explicit and persistable:

```cpp
config.Client.Addressing = AddressMode::Static;
config.Client.StaticNetwork.Address = IPv4Address(192, 168, 1, 50);
config.Client.StaticNetwork.Gateway = IPv4Address(192, 168, 1, 1);
config.Client.StaticNetwork.SubnetMask = IPv4Address(255, 255, 255, 0);
config.Client.StaticNetwork.PrimaryDNS = IPv4Address(1, 1, 1, 1);
```

AP network and DHCP-server settings are separate:

```cpp
config.AccessPoint.Network.Address = IPv4Address(192, 168, 10, 1);
config.AccessPoint.DHCP.Enabled = true;
config.AccessPoint.DHCP.LeaseStart = IPv4Address(192, 168, 10, 10);
config.AccessPoint.DHCP.LeaseEnd = IPv4Address(192, 168, 10, 100);
config.AccessPoint.DHCP.LeaseDurationSeconds = 7200;
```

The ESP32 Arduino backend applies the addressing settings supported by the selected framework; the complete configuration remains persisted even when a particular backend cannot tune every DHCP-server detail directly.

## Scanning

```cpp
wifi.OnScanCompleted([](const std::vector<ScanResult>& networks) {
    for (const auto& network : networks) {
        Serial.printf("%s RSSI=%d channel=%u\n",
            network.SSID.c_str(), network.RSSI, network.Channel);
    }
});

wifi.Scan();
```

Results contain only ESPressio types: SSID, BSSID, RSSI, channel and security mode.

## Direct callbacks

AP and Client contexts remain deliberately separate:

```cpp
wifi.OnClientStateChanged([](const ClientRuntimeState& before,
                             const ClientRuntimeState& after) {
    // Client-only transition.
});

wifi.OnAccessPointStateChanged([](const AccessPointRuntimeState& before,
                                  const AccessPointRuntimeState& after) {
    // AP-only transition.
});

wifi.OnAccessPointStationConnected([](const MacAddress& station) {
    // A station joined our AP.
});
```

`OnModeChanged()` reports overall topology changes independently of either subsystem.

## Observable notifications

Implement `IWiFiObserver` and register it with the same manager:

```cpp
class Observer : public IWiFiObserver {
public:
    void OnClientStateChanged(
        const ClientRuntimeState& before,
        const ClientRuntimeState& after
    ) override {
        // ...
    }
};

Observer observer;
auto handle = wifi.RegisterObserver(&observer);
```

Observable notifications include mode, AP state, Client state, scan lifecycle/results, AP station joins/leaves, and Client IP acquisition/loss.

## Optional Event bridge

```cpp
#include <ESPressio_WiFiEventBridge.hpp>

ESPressio::Event::WiFiEventBridge bridge;
bridge.Initialize(wifi);
```

The bridge converts Observable notifications into WiFi-owned ESPressio Event types. Event remains optional.

## Optional Command handler

```cpp
#include <ESPressio_WiFiCommandHandler.hpp>

WiFiCommandHandler commands;
commands.Initialize(
    ESPressio::Command::CommandRegistry::GetInstance(),
    wifi
);
```

This makes the same control surface usable from ESPressio Serial today and a future browser console later. Representative commands are:

```text
wifi status
wifi mode ap
wifi mode client
wifi mode ap-client
wifi scan
wifi client connect
wifi client disconnect
wifi client ssid "Studio"
wifi client password "secret"
wifi ap start
wifi ap stop
wifi ap ssid "ESPressio-Lab"
wifi ap password "secret"
wifi ap channel 6
```

There is intentionally no command for reading a password back. Credentials may be set, never emitted.

## Persisting configuration

Configuration is Serializable by design. Persistence remains opt-in and provider-neutral:

```cpp
#include <ESPressio_WiFiPersistence.hpp>

WiFiConfigurationStore::Save(storage, "/wifi.espb", config);
WiFiConfigurationStore::Load(storage, "/wifi.espb", config);
```

The same helpers accept `IKeyValueStorage`, so Preferences/NVS works without WiFi knowing that NVS exists.

## Protecting the complete configuration

Passwords are marked Sensitive for redaction, but redaction is not encryption. For persisted credentials, authenticated protection is strongly recommended:

```cpp
#include <ESPressio_WiFiPersistenceSecurity.hpp>

Serializable::SerializationProtectionConfig protection;
protection.Protector = &protector;
protection.Context = "ESPressio.WiFi.Configuration";

ProtectedWiFiConfigurationStore::Save(
    storage,
    "/wifi.esdp",
    config,
    protection
);
```

Protection covers the entire serialized configuration, not just passwords. WiFi never chooses the cipher or owns key material; those responsibilities remain in ESPressio Security.

## Compile-time extensible configuration

Applications can persist WiFi settings together with application-specific settings without modifying ESPressio WiFi.

```cpp
#include <ESPressio_ExtensibleWiFiConfiguration.hpp>

struct MyNetworkExtras final
    : ESPressio::Serializable::Serializable<MyNetworkExtras> {
    ESPRESSIO_SERIALIZABLE_TYPE(MyNetworkExtras)
    uint16_t DiscoveryPort = 9000;
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("discoveryPort", DiscoveryPort)
    )
};

using MyConfiguration = WiFiConfigurationWith<MyNetworkExtras>;

ConfiguredWiFiManager<MyConfiguration> wifi(platform);
MyConfiguration config;
config.WiFi.AccessPoint.SSID = "Camera-Control";
config.Extension.DiscoveryPort = 9100;
wifi.Configure(config);
```

`WiFiConfigurationStore` and `ProtectedWiFiConfigurationStore` are templated, so the whole extended object can be persisted/protected as one document. The hardware implementation receives only the standard `config.WiFi` section.

## Dependency direction

```text
WiFi 0.1.0
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.0 < 1.0.0

optional
    - - -> Persistence >= 0.3.0 < 1.0.0
    - - -> Security >= 0.4.0 < 1.0.0 (through protected serialization)
    - - -> Event >= 6.0.1 < 7.0.0
    - - -> Command >= 1.0.1 < 2.0.0
```

Serial may consume WiFi, never the reverse. Web infrastructure is intentionally excluded.

See `ESPRESSIO_DEPENDENCY_CHART.md` and `CHANGELOG.md` for the coordinated platform position and release history.
