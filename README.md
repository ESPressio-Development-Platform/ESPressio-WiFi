# ESPressio WiFi

Autonomous, platform-neutral WiFi lifecycle and configuration for ESP32 applications.

## Version — 0.2.0

0.2.0 adds three core capabilities:

- **remembered Client networks with deterministic priority-based automatic selection and failover**;
- **autonomous WiFi runtime servicing on ESPressio Threads `PrecisionThread`**, removing the need to call `wifi.Poll()` from the application loop;
- **`APUntilClient` IoT fallback mode**, which exposes an AP only while Client connectivity is unavailable.

WiFi 0.2.0 is validated against the corrected Serializable 0.11.2 cascade: Observable 3.0.2, Serializable 0.11.2, Units 0.2.6, Timing 2.2.7, Threads 3.1.6, Event 6.0.2, Command 1.0.2, Security 0.4.1 and Persistence 0.3.1.

## What ESPressio WiFi owns

- Access Point, Client, AP+Client and AP-until-Client operating modes.
- Independent AP and Client runtime state machines.
- ESPressio-owned IPv4/MAC/network/security/scan types; no Arduino/ESP-IDF types in public APIs.
- Always-Serializable WiFi configuration.
- Multiple remembered Client network profiles, including credentials, addressing and priority.
- Automatic scan → select → connect → failover behaviour.
- Conditional AP fallback and remembered-network retry behaviour for IoT provisioning/recovery.
- DHCP/static Client addressing and AP DHCP-server configuration data.
- Asynchronous scanning.
- Direct callbacks and ESPressio Observable notifications.
- An explicit ESP32 implementation behind `IWiFiPlatform`.
- Autonomous runtime execution through ESPressio Threads `PrecisionThread`.
- Optional Persistence, protected Persistence/Security, Event and Command integrations.

HTTP, Captive Portal, WebSocket, browser UI and other Web concerns deliberately belong in ESPressio Web rather than this library. WiFi owns the lifecycle that makes those facilities reachable; Web owns the user interface.

## Installation

Core WiFi 0.2.0:

```ini
lib_deps =
    espressio-development-platform/ESPressio-WiFi@^0.2.0
    espressio-development-platform/ESPressio-Observable@^3.0.2
    espressio-development-platform/ESPressio-Serializable@^0.11.2
    espressio-development-platform/ESPressio-Threads@^3.1.6
```

Add Persistence, Security, Event and Command only when selecting those integrations.

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

With the default selection policy, ESPressio WiFi automatically scans when Client operation starts, matches visible SSIDs against remembered profiles, ignores disabled/unknown profiles, chooses the highest priority visible profile, uses strongest RSSI to break equal-priority ties, chooses the strongest BSSID for duplicate SSIDs, and advances to the next eligible remembered profile when `TryNextOnFailure` is enabled.

A healthy current Client connection is intentionally **sticky**. A background/manual scan does not disconnect a working network merely because a higher-priority remembered network appears.

## `APUntilClient` — IoT provisioning/recovery mode

`APUntilClient` is intended for devices that should normally join an existing WiFi network, but must remain directly reachable when no usable Client network is available.

```text
remembered networks exist
        |
        v
   STA-only startup
        |
        +---- Client connects ----------------------> STA-only
        |
        +---- fallback timeout expires
                         |
                         v
                    AP + STA
                    fallback AP
                         |
                         +---- periodic scan/retry
                         |
                         +---- Client connects ------> stop AP -> STA-only
                                                       |
                                                       +---- later Client loss
                                                               |
                                                               v
                                                         STA-only retry
                                                               |
                                                               +---- timeout -> AP+STA fallback again
```

If there are **no remembered networks**, the fallback AP starts immediately because there is nothing useful for STA to attempt.

### Basic `APUntilClient` configuration

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
    config.Mode = WiFiMode::APUntilClient;

    config.AccessPoint.Enabled = true;
    config.AccessPoint.SSID = "ESPressio-Setup";
    config.AccessPoint.Password = "setup-password";

    config.Client.Enabled = true;

    ClientNetworkProfile home;
    home.SSID = "Home";
    home.Password = "home-password";
    home.Priority = 300;

    ClientNetworkProfile studio;
    studio.SSID = "Studio";
    studio.Password = "studio-password";
    studio.Priority = 200;

    config.Client.Networks = { home, studio };

    wifi.Configure(config);
    wifiWorker.Initialize();
    wifiWorker.Start();
}

void loop() {
    // No WiFi polling required.
}
```

At startup WiFi remains STA-only while it scans and attempts remembered networks. If no usable Client connection is established before the fallback timeout, the ESP32 transitions to AP+STA and exposes `ESPressio-Setup` while continuing to look for remembered networks.

### Configurable fallback and retry timing

```cpp
config.APUntilClient.FallbackTimeoutMilliseconds = 30'000;
config.APUntilClient.RetryScanIntervalMilliseconds = 30'000;
```

The defaults are 30 seconds for both values. `FallbackTimeoutMilliseconds` does not apply when there are zero remembered networks: the AP starts immediately. While fallback is active, retries occur in AP+STA so provisioning/control clients are not deliberately dropped merely because STA is scanning again.

### Runtime provisioning

A future ESPressio Web captive portal, a Serial console, or application code can all use the same WiFi-owned remembered-network API:

```cpp
ClientNetworkProfile network;
network.SSID = "New-Site-WiFi";
network.Password = "new-site-password";
network.Priority = 500;

wifi.AddOrUpdateClientNetwork(network);
wifi.SaveConfiguration();
```

In `APUntilClient`, adding or updating a remembered profile triggers an immediate scan/connection attempt. The fallback AP remains available until Client connectivity is actually established, then WiFi shuts the AP down and returns to STA-only.

Equivalent Commands:

```text
wifi mode ap-until-client
wifi client networks add "New-Site-WiFi" "new-site-password" 500
wifi config save
```

Fallback controls:

```text
wifi ap-until-client status
wifi ap-until-client retry-now
wifi ap-until-client fallback-timeout 30000
wifi ap-until-client retry-interval 30000
```

### Observing `APUntilClient`

`WiFiRuntimeState::APUntilClient` exposes `Inactive`, `SeekingClient`, `FallbackAccessPoint`, and `ClientConnected` phases plus fallback/retry timing.

Direct callback:

```cpp
wifi.OnAPUntilClientStateChanged([](
    const APUntilClientRuntimeState& before,
    const APUntilClientRuntimeState& after
) {
    Serial.printf(
        "APUntilClient state %u -> %u, fallback AP=%s\n",
        static_cast<unsigned>(before.State),
        static_cast<unsigned>(after.State),
        after.FallbackAccessPointActive ? "active" : "inactive"
    );
});
```

`IWiFiObserver` exposes the equivalent observer callback. When Event integration is selected, `WiFiEventBridge` registers itself as a WiFi Observer and emits `WiFiAPUntilClientStateChangedEvent` for asynchronous subscribers.

### `APUntilClient` vs `AccessPointClient`

| Mode | Client | Access Point |
| --- | --- | --- |
| `Client` | active | off |
| `AccessPoint` | off | always active |
| `AccessPointClient` | active | **always active** |
| `APUntilClient` | active | **only while Client connectivity is unavailable** |

`AccessPointClient` never shuts its AP down merely because the Client connects. `APUntilClient` does exactly that by design.

## Selection policy

```cpp
config.Client.Selection.AutomaticSelection = true;
config.Client.Selection.ScanOnStartup = true;
config.Client.Selection.ScanOnDisconnect = true;
config.Client.Selection.TryNextOnFailure = true;
```

All four options default to the behaviour shown above.

## Static addressing per remembered network

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

## Permanent AP + Client

```cpp
config.Mode = WiFiMode::AccessPointClient;
config.AccessPoint.Enabled = true;
config.AccessPoint.SSID = "ESPressio-Control";
config.AccessPoint.Password = "control-password";
config.Client.Enabled = true;
```

`WiFiRuntimeState` reports AP and Client state independently.

## Observing remembered-network selection

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

`IWiFiObserver` exposes `OnClientNetworkSelectionChanged(...)`, `OnClientNetworkSelected(...)`, and `OnClientNoKnownNetworkAvailable()`. The optional WiFi Event bridge emits corresponding Serializable Events.

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

The worker services scan completion automatically; no `Poll()` call is required.

## Managing remembered networks

```cpp
ClientNetworkProfile profile;
profile.SSID = "Workshop";
profile.Password = "workshop-password";
profile.Priority = 250;

wifi.AddOrUpdateClientNetwork(profile);
wifi.SetClientNetworkPriority("Workshop", 400);
wifi.RemoveClientNetwork("Old-Network");
```

Call `SaveConfiguration()` when configuration changes should survive reboot.

## Optional Command handler

```text
wifi status
wifi mode ap-until-client
wifi scan
wifi scan results

wifi client status
wifi client auto-select true
wifi client networks list
wifi client networks add "Home" "home-password" 300
wifi client networks add "Studio" "studio-password" 200
wifi client networks priority "Studio" 400
wifi client networks remove "Old-Network"

wifi ap-until-client status
wifi ap-until-client retry-now
wifi ap-until-client fallback-timeout 30000
wifi ap-until-client retry-interval 30000

wifi config save
wifi config load
```

`wifi client networks list` reports SSID, priority, enabled state and addressing mode but **never returns plaintext passwords**.

## Persistence

The complete `WiFiConfiguration`, including remembered profiles and `APUntilClient` timing, can be stored through developer-selected ESPressio Persistence providers:

```cpp
#include <ESPressio_WiFiPersistence.hpp>

WiFiConfigurationStore::Save(storage, "/wifi.espb", config);
WiFiConfigurationStore::Load(storage, "/wifi.espb", config);
```

WiFi 0.2.0 validates this optional integration against Persistence 0.3.1.

## Protecting remembered credentials

Each password is Sensitive/redacted Serializable data. For persisted credentials, authenticated whole-configuration protection is strongly recommended:

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

Protected persistence is validated against Serializable 0.11.2, Persistence 0.3.1 and Security 0.4.1. WiFi never chooses the cipher or owns key material.

## Thread safety

WiFi 0.2.0 treats `WiFiManager` as a concurrently accessed service. Configuration, runtime state, scan results, remembered profiles and selection state are synchronized internally.

Callbacks and Observers are invoked **after internal state locks are released**, so notifications may safely call back into WiFi without being invoked beneath the manager state mutex. `Configuration()`, `State()`, `LastScanResults()` and `EligibleClientNetworks()` return snapshots rather than exposing mutable internal references.

## `ProcessOnce()` and legacy `Poll()`

`WiFiManager::ProcessOnce()` remains public for deterministic tests and specialist integrations. `Poll()` is retained as a 0.1.x compatibility alias. Normal applications should use `WiFiWorker` and should not service either function manually.

## Dependencies

```text
WiFi 0.2.0
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.2 < 1.0.0
    -> Threads >= 3.1.6 < 4.0.0

optional
    - - -> Persistence >= 0.3.1 < 1.0.0
    - - -> Security >= 0.4.1 < 1.0.0
    - - -> Event >= 6.0.2 < 7.0.0
    - - -> Command >= 1.0.2 < 2.0.0
```

Threads is required because autonomous WiFi servicing is core 0.2.0 behaviour. Event, Command, Persistence and Security remain opt-in. Serial may consume WiFi, never the reverse. Web infrastructure is intentionally excluded.

See `ESPRESSIO_DEPENDENCY_CHART.md` and `CHANGELOG.md` for the coordinated platform position and release history.
