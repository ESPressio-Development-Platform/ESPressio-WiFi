# ESPressio WiFi

Autonomous, platform-neutral WiFi lifecycle and configuration. Target-specific implementations of the WiFi platform contract are supplied by platform packages such as ESPressio-ESP32.

The canonical branch for the Primitive Platform Redesign is `primitives_redesign`.

## Core capabilities

- remembered Client networks with deterministic priority-based automatic selection and failover;
- autonomous WiFi runtime servicing on one final ESPressio `ThreadWith<Precision<...>>` root, removing application-loop polling;
- `APUntilClient` IoT fallback mode, exposing an AP only while Client connectivity is unavailable;
- ESPressio-owned IPv4/MAC/network/security/scan types with no Arduino/ESP-IDF types in public APIs;
- Serializable WiFi configuration, direct callbacks and Observable notifications;
- a platform-neutral `IWiFiPlatform` contract implemented by target packages;
- optional downstream Persistence/Security/Event/Command integration without changing WiFi ownership boundaries.

HTTP, Captive Portal, WebSocket, browser UI and other Web concerns belong in ESPressio-Web. WiFi owns the connectivity lifecycle that makes those facilities reachable; Web owns the user-facing protocol/UI layer.

## Installation

```ini
lib_deps =
    https://github.com/ESPressio-Development-Platform/ESPressio-WiFi.git#primitives_redesign
    https://github.com/ESPressio-Development-Platform/ESPressio-System.git#primitives_redesign
    https://github.com/ESPressio-Development-Platform/ESPressio-Observable.git#primitives_redesign
    https://github.com/ESPressio-Development-Platform/ESPressio-Serializable.git#primitives_redesign
    https://github.com/ESPressio-Development-Platform/ESPressio-Threads.git#primitives_redesign
```

On ESP32, also add ESPressio-ESP32 from `primitives_redesign`; it supplies the concrete `WiFiPlatform` implementation. Add optional family libraries only when selecting their corresponding integrations.

## Autonomous worker — no polling required

```cpp
#include <ESPressio_WiFi.hpp>
#include <ESPressio_WiFiWorker.hpp>
#include <ESPressio_WiFiPlatform.hpp> // provided by ESPressio-ESP32

using namespace ESPressio::WiFi;

WiFiPlatform platform;
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

`WiFiWorker` is a normal final `Threads::ThreadWith<Threads::Precision<8>>`. Precision is a resident capability, not a specialized Thread class. The worker owns no private scheduler, private wake signal, or secondary task. WiFi work signals call `Precision::Bump()`, which publishes immediate application eligibility through the root Thread's one common wake path.

Default service period is 50 ms and default desired execution budget is 5 ms. Runtime tuning remains separate from persisted WiFi configuration:

```cpp
WiFiWorkerConfiguration runtime;
runtime.IterationPeriodMilliseconds = 25;
runtime.DesiredExecutionBudgetMilliseconds = 5;

WiFiWorker wifiWorker(wifi, runtime);
```

Thread lifecycle, cadence, wake semantics, skipped-iteration accounting and resource diagnostics remain owned by ESPressio-Threads.

## Remembered Client networks

Client mode can maintain multiple profiles. Each profile contains its own SSID, sensitive password, priority, enabled state and DHCP/static addressing configuration.

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

config.Client.Networks = { home, studio };
wifi.Configure(config);
wifiWorker.Initialize();
wifiWorker.Start();
```

Automatic selection scans when required, matches visible SSIDs against remembered profiles, ignores disabled/unknown profiles, selects the highest-priority visible profile, uses strongest RSSI to break equal-priority ties, and can advance to the next eligible profile after failure. A healthy current connection is sticky; a scan does not disconnect it merely because a higher-priority profile appears.

## `APUntilClient`

`APUntilClient` is for devices that should normally join existing infrastructure but remain directly reachable while no usable Client network is available.

```text
STA startup -> scan/connect attempts
    | success                       | fallback timeout/no remembered network
    v                               v
 STA-only                       AP + STA fallback
    ^                               |
    +-------- Client connects ------+
```

```cpp
WiFiConfiguration config;
config.Mode = WiFiMode::APUntilClient;
config.AccessPoint.Enabled = true;
config.AccessPoint.SSID = "ESPressio-Setup";
config.AccessPoint.Password = "setup-password";
config.Client.Enabled = true;
config.APUntilClient.FallbackTimeoutMilliseconds = 30'000;
config.APUntilClient.RetryScanIntervalMilliseconds = 30'000;
```

If no remembered networks exist, the fallback AP starts immediately. While fallback is active, retries occur without deliberately dropping provisioning/control clients. Once Client connectivity succeeds, WiFi shuts the fallback AP down and returns to STA-only operation.

## Runtime provisioning

A Web UI, Serial tool, Command handler or application code can use the same WiFi-owned profile API:

```cpp
ClientNetworkProfile network;
network.SSID = "New-Site-WiFi";
network.Password = "new-site-password";
network.Priority = 500;

wifi.AddOrUpdateClientNetwork(network);
wifi.SaveConfiguration();
```

In `APUntilClient`, profile changes can trigger immediate selection/connection work through the worker's common wake path.

## Observation

`IWiFiObserver` and direct callback surfaces expose lifecycle, scan, selection and `APUntilClient` state without exposing platform-native WiFi types. Optional Event integration may translate those public observations using the final Event family contracts; the bridge does not alter WiFi ownership or execution semantics.

Serial diagnostics consume these public WiFi surfaces downstream. Credentials are never returned merely because diagnostics or profile listing is enabled.

## Persistence and credential protection

The complete `WiFiConfiguration` can be stored through the selected ESPressio-Persistence provider. Password fields remain Sensitive/redacted Serializable data. Authenticated whole-configuration protection should be used for persisted credentials when the Security integration is selected; WiFi never chooses the cipher or owns key material.

All redesign dependencies and focused integration workflows use `primitives_redesign`. No version number is changed by this tranche.

## Thread safety

`WiFiManager` is a concurrently accessed service. Configuration, runtime state, scan results, remembered profiles and selection state are synchronized internally. Callbacks and Observers are invoked after internal state locks are released, and snapshot accessors do not expose mutable internal state.

`WiFiManager::ProcessOnce()` remains public for deterministic tests and specialist integrations. Normal applications use `WiFiWorker`; no application polling is required.

## Dependency model

```text
WiFi
    -> System
    -> Observable
    -> Serializable
    -> Threads

platform integration
    - - -> ESPressio-ESP32

optional downstream integration
    - - -> Persistence
    - - -> Security
    - - -> Event
    - - -> Command
```

Threads is required because autonomous WiFi servicing is core behavior. The concrete ESP32 implementation remains in ESPressio-ESP32. Serial may consume WiFi, never the reverse. Web infrastructure is intentionally excluded from the portable WiFi core.

## Design invariants

- `WiFiWorker` uses final generic Thread/capability composition; removed specialized Thread classes are not part of the API.
- The worker contributes exactly one root execution context and one common wake path.
- Target-specific WiFi APIs remain behind `IWiFiPlatform`.
- Event/Command/Persistence/Security integrations are optional and cannot invert the core dependency graph.
- Sensitive credentials are not exposed through diagnostics or generic tooling.
- Runtime configuration and persisted network configuration remain separate concerns.
