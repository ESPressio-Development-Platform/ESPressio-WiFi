# ESPressio WiFi

Autonomous, platform-neutral WiFi lifecycle and configuration for ESP32 applications.

## Version — 0.2.0

0.2.0 adds three core capabilities:

- **remembered Client networks with deterministic priority-based automatic selection and failover**;
- **autonomous WiFi runtime servicing on ESPressio Threads `PrecisionThread`**, removing the need to call `wifi.Poll()` from the application loop;
- **`APUntilClient` IoT fallback mode**, which exposes an AP only while Client connectivity is unavailable.

## What ESPressio WiFi owns

- Access Point, Client, AP+Client and AP-until-Client operating modes.
- Independent AP and Client runtime state machines.
- ESPressio-owned IPv4/MAC/network/security/scan types; no Arduino/ESP-IDF types in public APIs.
- Always-Serializable WiFi configuration.
- Multiple remembered Client network profiles, including credentials, addressing and preference priority.
- Automatic scan → select → connect → failover behaviour.
- Conditional AP fallback and remembered-network retry behaviour for IoT provisioning/recovery.
- DHCP/static Client addressing and AP DHCP-server configuration data.
- asynchronous scanning.
- direct callbacks and ESPressio Observable notifications.
- an explicit ESP32 implementation behind `IWiFiPlatform`.
- autonomous runtime execution through ESPressio Threads `PrecisionThread`.
- optional Persistence, protected Persistence/Security, Event and Command integrations.

HTTP, Captive Portal, WebSocket, browser UI and other Web concerns deliberately belong in ESPressio Web rather than this library. WiFi owns the lifecycle that makes those facilities reachable; Web owns the user interface.

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

## `APUntilClient` — recommended IoT provisioning/recovery mode

`APUntilClient` is intended for devices that should normally join an existing WiFi network, but must remain directly reachable when no usable Client network is available.

The lifecycle is deliberately different from permanent AP+Client mode:

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

If there are **no remembered networks at all**, the fallback AP is started immediately because there is nothing useful for STA to attempt.

### Basic `APUntilClient` example

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

    // The fallback AP used for provisioning/recovery.
    config.AccessPoint.Enabled = true;
    config.AccessPoint.SSID = "ESPressio-Setup";
    config.AccessPoint.Password = "setup-password";

    // Client operation uses remembered networks.
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

At startup, WiFi remains STA-only while it scans and attempts remembered networks. If neither network can be used before the fallback timeout, the ESP32 switches to AP+STA and exposes `ESPressio-Setup` while continuing to look for a remembered network.

### Configuring fallback and retry timing

The timing configuration is Serializable and persists with the rest of `WiFiConfiguration`:

```cpp
config.APUntilClient.FallbackTimeoutMilliseconds = 30'000;
config.APUntilClient.RetryScanIntervalMilliseconds = 30'000;
```

The defaults are 30 seconds for both values.

`FallbackTimeoutMilliseconds` controls how long a device with remembered networks is allowed to keep trying Client connectivity before the fallback AP is enabled. It does **not** apply when there are zero remembered networks: in that case the AP starts immediately.

`RetryScanIntervalMilliseconds` controls how frequently remembered networks are scanned again while the fallback AP is active. During these retries the radio remains in AP+STA mode, so provisioning/control clients are not deliberately disconnected simply because the device is looking for infrastructure WiFi.

### Provisioning a device while its fallback AP is active

A future ESPressio Web captive portal can call the same WiFi-owned API that Serial/Command integrations use. WiFi itself does not care how the operator supplied the credentials.

```cpp
ClientNetworkProfile network;
network.SSID = "New-Site-WiFi";
network.Password = "new-site-password";
network.Priority = 500;

wifi.AddOrUpdateClientNetwork(network);
wifi.SaveConfiguration();
```

In `APUntilClient` mode, adding or updating a remembered profile triggers an immediate scan/connection attempt. The fallback AP remains available until Client connectivity is actually established. Once the Client connects successfully, ESPressio WiFi shuts the AP down and returns the ESP32 radio to STA-only operation.

The same flow can be driven through Commands:

```text
wifi mode ap-until-client
wifi client networks add "New-Site-WiFi" "new-site-password" 500
wifi config save
```

Fallback timing and explicit retry are also available through Commands:

```text
wifi ap-until-client status
wifi ap-until-client retry-now
wifi ap-until-client fallback-timeout 30000
wifi ap-until-client retry-interval 30000
```

`retry-now` is useful after an external provisioning/control workflow has changed surrounding network conditions or when an operator explicitly wants to retry without waiting for the periodic retry interval.

### Observing the `APUntilClient` lifecycle

Consumers do not need to infer fallback state from separate AP and Client transitions. `WiFiRuntimeState::APUntilClient` exposes a dedicated lifecycle snapshot with `Inactive`, `SeekingClient`, `FallbackAccessPoint`, and `ClientConnected` states, plus the current fallback deadline and next retry time.

For a direct synchronous callback:

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

Observer-based consumers can override the equivalent callback:

```cpp
void OnAPUntilClientStateChanged(
    const APUntilClientRuntimeState& before,
    const APUntilClientRuntimeState& after
) override {
    // React synchronously to the WiFi lifecycle transition.
}
```

When the optional Event integration is used, `WiFiEventBridge` registers itself as a WiFi Observer and translates this callback into a Serializable `WiFiAPUntilClientStateChangedEvent`. Asynchronous Event subscribers can therefore react to fallback activation, Client acquisition, or recovery transitions without holding a reference to `WiFiManager`.

### `APUntilClient` vs `AccessPointClient`

Choose the mode according to whether the AP is a permanent service or a fallback/recovery mechanism:

| Mode | Client | Access Point |
| --- | --- | --- |
| `Client` | active | off |
| `AccessPoint` | off | always active |
| `AccessPointClient` | active | **always active** |
| `APUntilClient` | active | **active only while Client connectivity is unavailable** |

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

## Permanent AP + Client

Remembered-network selection works unchanged while the ESP32 is simultaneously hosting a permanent Access Point:

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

The legacy 0.1.x single-network commands remain available for source compatibility, but new applications should prefer remembered profiles.

## Persisting remembered networks

The complete `WiFiConfiguration`, including remembered profiles and `APUntilClient` timing, remains Serializable and can be stored through any developer-supplied ESPressio Persistence provider:

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

Threads is a required dependency because autonomous WiFi servicing is a core 0.2.0 capability. WiFi does not create or manage a private FreeRTOS task outside the ESPressio Threads lifecycle.

Serial may consume WiFi, never the reverse. Web infrastructure is intentionally excluded.

See `ESPRESSIO_DEPENDENCY_CHART.md` and `CHANGELOG.md` for the coordinated platform position and release history.
