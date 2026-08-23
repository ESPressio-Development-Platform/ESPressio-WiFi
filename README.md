# ESPressio WiFi

Autonomous, platform-neutral WiFi lifecycle and configuration for ESP32 applications.

## Version — 0.2.0

0.2.0 adds two core capabilities:

- **remembered Client networks with deterministic priority-based automatic selection and failover**;
- **autonomous WiFi runtime servicing on ESPressio Threads `PrecisionThread`**, removing the need to call `wifi.Poll()` from the application loop.

## What ESPressio WiFi owns

- Access Point, Client and AP+Client operating modes.
- Independent AP and Client runtime state machines.
- ESPressio-owned IPv4/MAC/network/security/scan types; no Arduino/ESP-IDF types in public APIs.
- Always-Serializable WiFi configuration.
- Multiple remembered Client network profiles, including credentials, addressing and preference priority.
- Automatic scan → select → connect → failover behaviour.
- DHCP/static Client addressing and AP DHCP-server configuration data.
- asynchronous scanning.
- direct callbacks and ESPressio Observable notifications.
- an explicit ESP32 implementation behind `IWiFiPlatform`.
- autonomous runtime execution through ESPressio Threads `PrecisionThread`.
- optional Persistence, protected Persistence/Security, Event and Command integrations.

HTTP, WebSocket, browser UI and other Web concerns deliberately belong in ESPressio Web rather than this library.

## Minimal Access Point — no polling required

```cpp
#include <ESPressio_WiFi.hpp>
#include <ESPressio_WiFiWorker.hpp>
#include <ESPressio_ESP32WiFi.hpp>

using namespace ESPressio::WiFi;

ESP32WiFiPlatform platform;
WiFiManager wifi(platform);
WiFiWorker wifiWorker(wifi);

void setup() {
    WiFiConfiguration config;
    config.Mode = WiFiMode::AccessPoint;
    config.AccessPoint.SSID = "ESPressio-Device";
    config.AccessPoint.Password = "change-me";

    wifi.Configure(config);
    wifiWorker.Initialize();
    wifiWorker.Start();
}

void loop() {
    // ESPressio WiFi requires no application polling.
}
```

`WiFiWorker` is implemented with ESPressio Threads `PrecisionThread`. Its default service period is 50 ms and its default desired execution budget is 5 ms. Explicit operations such as scans, connects and configuration changes bump the worker so they do not need to wait for the next scheduled iteration.

Runtime scheduling is intentionally separate from persisted WiFi configuration:

```cpp
WiFiWorkerConfiguration runtime;
runtime.IterationPeriodMilliseconds = 25;
runtime.DesiredExecutionBudgetMilliseconds = 5;

WiFiWorker wifiWorker(wifi, runtime);
```

Thread lifecycle, rate limiting, skipped-iteration accounting and diagnostics remain owned by ESPressio Threads.

## Remembered Client networks

Client mode can persist multiple network profiles. Each profile contains its own SSID, sensitive password, priority, enabled state and DHCP/static addressing configuration.

```cpp
WiFiConfiguration config;
config.Mode = WiFiMode::Client;
config.Client.Enabled = true;

ClientNetworkProfile home;
home.SSID = "Home";
home.Password = "home-password";
home.Priority = 300;

ClientNetworkProfile studio;
studio.SSID = "Studio";
studio.Password = "studio-password";
studio.Priority = 200;

ClientNetworkProfile hotspot;
hotspot.SSID = "Phone-Hotspot";
hotspot.Password = "hotspot-password";
hotspot.Priority = 100;

config.Client.Networks = { home, studio, hotspot };

wifi.Configure(config);
wifiWorker.Initialize();
wifiWorker.Start();
```

With the default selection policy, ESPressio WiFi automatically:

1. scans when Client operation starts;
2. matches visible SSIDs against remembered profiles;
3. ignores disabled or unknown profiles;
4. sorts available known networks by **highest `Priority` first**;
5. uses strongest RSSI as the tie-breaker between equal-priority profiles;
6. when multiple BSSIDs advertise the same remembered SSID, uses the strongest visible BSSID for selection context;
7. connects using the selected profile's credentials and addressing configuration;
8. if the selected profile ultimately fails and `TryNextOnFailure` is enabled, advances to the next eligible profile;
9. when disconnected and `ScanOnDisconnect` is enabled, scans again and repeats selection.

A healthy existing connection is intentionally **sticky**. A background/manual scan does not disconnect a working network merely because a higher-priority remembered network has appeared.

## Selection policy

```cpp
config.Client.Selection.AutomaticSelection = true;
config.Client.Selection.ScanOnStartup = true;
config.Client.Selection.ScanOnDisconnect = true;
config.Client.Selection.TryNextOnFailure = true;
```

All four options default to the behaviour shown above.

## Static addressing per remembered network

Addressing belongs to the profile because different networks may require different static settings:

```cpp
ClientNetworkProfile cameraLAN;
cameraLAN.SSID = "Camera-LAN";
cameraLAN.Password = "camera-password";
cameraLAN.Priority = 500;
cameraLAN.Addressing = AddressMode::Static;
cameraLAN.StaticNetwork.Address = IPv4Address(192, 168, 50, 20);
cameraLAN.StaticNetwork.Gateway = IPv4Address(192, 168, 50, 1);
cameraLAN.StaticNetwork.SubnetMask = IPv4Address(255, 255, 255, 0);
cameraLAN.StaticNetwork.PrimaryDNS = IPv4Address(1, 1, 1, 1);
```

DHCP remains the default for every profile.

## AP + Client

Remembered-network selection works unchanged while the ESP32 is simultaneously hosting an Access Point:

```cpp
config.Mode = WiFiMode::AccessPointClient;
config.AccessPoint.Enabled = true;
config.AccessPoint.SSID = "ESPressio-Control";
config.AccessPoint.Password = "control-password";
config.Client.Enabled = true;
```

`WiFiRuntimeState` continues to report AP and Client state independently.

## Observing selection

Direct callbacks expose the selection lifecycle:

```cpp
wifi.OnClientNetworkSelected([](const ClientNetworkCandidate& selected) {
    Serial.printf(
        "Selected %s priority=%u RSSI=%d\n",
        selected.SSID.c_str(),
        selected.Priority,
        selected.RSSI
    );
});

wifi.OnClientNoKnownNetworkAvailable([]() {
    Serial.println("No remembered network is currently visible");
});
```

`IWiFiObserver` exposes equivalent callbacks:

- `OnClientNetworkSelectionChanged(...)`
- `OnClientNetworkSelected(...)`
- `OnClientNoKnownNetworkAvailable()`

The optional WiFi Event bridge emits corresponding Serializable Events.

## Scanning

Manual scans are still available:

```cpp
wifi.OnScanCompleted([](const std::vector<ScanResult>& networks) {
    for (const auto& network : networks) {
        Serial.printf("%s RSSI=%d channel=%u\n",
            network.SSID.c_str(), network.RSSI, network.Channel);
    }
});

wifi.Scan();
```

The worker services scan completion automatically. No `Poll()` call is required.

## Managing remembered networks at runtime

The manager provides strongly typed helpers:

```cpp
ClientNetworkProfile profile;
profile.SSID = "Workshop";
profile.Password = "workshop-password";
profile.Priority = 250;

wifi.AddOrUpdateClientNetwork(profile);
wifi.SetClientNetworkPriority("Workshop", 400);
wifi.RemoveClientNetwork("Old-Network");
```

Call `SaveConfiguration()` after configuration changes when they should survive reboot.

## Optional Command handler

The WiFi-owned Command handler exposes the same profile functionality to Serial today and future Web consoles later:

```text
wifi status
wifi scan
wifi scan results

wifi client status
wifi client auto-select true
wifi client networks list
wifi client networks add "Home" "home-password" 300
wifi client networks add "Studio" "studio-password" 200
wifi client networks priority "Studio" 400
wifi client networks remove "Old-Network"

wifi config save
wifi config load
```

`wifi client networks list` reports SSID, priority, enabled state and addressing mode but **never returns plaintext passwords**.

The legacy 0.1.x single-network commands remain available for source compatibility, but new applications should prefer remembered profiles.

## Persisting remembered networks

The complete `WiFiConfiguration`, including every remembered profile and its priority/addressing settings, remains Serializable and can be stored through any developer-supplied ESPressio Persistence provider:

```cpp
#include <ESPressio_WiFiPersistence.hpp>

WiFiConfigurationStore::Save(storage, "/wifi.espb", config);
WiFiConfigurationStore::Load(storage, "/wifi.espb", config);
```

Preferences/NVS, LittleFS, SPIFFS, FFat, SD and future Persistence providers remain interchangeable at the WiFi layer.

## Protecting remembered credentials

Each password is marked Sensitive for diagnostic redaction. For persisted credentials, authenticated whole-configuration protection is strongly recommended:

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

Protection covers all remembered credentials and all other configuration fields. WiFi never chooses the cipher or owns key material.

## Thread safety

0.2.0 treats WiFi as a concurrently accessed service. Configuration, runtime state, scan results, remembered profiles and selection state are synchronized inside `WiFiManager`.

Callbacks and Observers are invoked **after internal state locks are released**, so a notification may safely call back into WiFi without being invoked beneath the manager's state mutex.

`Configuration()`, `State()`, `LastScanResults()` and `EligibleClientNetworks()` return snapshots rather than exposing mutable internal references.

## `ProcessOnce()` and legacy `Poll()`

`WiFiManager::ProcessOnce()` remains public primarily for deterministic host tests and specialist integrations. `Poll()` is retained as a 0.1.x compatibility alias.

Normal application code should use `WiFiWorker` and should not service either function manually.

## Dependencies

```text
WiFi 0.2.0
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.0 < 1.0.0
    -> Threads >= 3.1.5 < 4.0.0

optional
    - - -> Persistence >= 0.3.0 < 1.0.0
    - - -> Security >= 0.4.0 < 1.0.0
    - - -> Event >= 6.0.1 < 7.0.0
    - - -> Command >= 1.0.1 < 2.0.0
```

Threads is now a required dependency because autonomous WiFi servicing is a core 0.2.0 capability. WiFi does not create or manage a private FreeRTOS task outside the ESPressio Threads lifecycle.

Serial may consume WiFi, never the reverse. Web infrastructure is intentionally excluded.

See `ESPRESSIO_DEPENDENCY_CHART.md` and `CHANGELOG.md` for the coordinated platform position and release history.
