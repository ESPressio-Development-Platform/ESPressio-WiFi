# ESPressio WiFi

Autonomous, platform-neutral WiFi lifecycle and configuration for ESP32 applications.

## Version — 0.2.1

WiFi 0.2.1 is a dependency-maintenance release of the 0.2 generation. It preserves remembered Client network selection/failover, autonomous `PrecisionThread` servicing and `APUntilClient` provisioning/recovery behaviour while moving all integration validation onto the released Serializable 0.11.3 cascade.

Validated generation:

```text
Observable    3.0.2
Serializable  0.11.3
Units         0.2.7
Timing        2.2.8
Threads       3.1.7
Persistence   0.3.2
Security      0.4.2
Event         6.0.3
Command       1.0.3
WiFi          0.2.1
```

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

```ini
lib_deps =
    espressio-development-platform/ESPressio-WiFi@^0.2.1
    espressio-development-platform/ESPressio-Observable@^3.0.2
    espressio-development-platform/ESPressio-Serializable@^0.11.3
    espressio-development-platform/ESPressio-Threads@^3.1.7
```

Add Persistence, Security, Event and Command only when selecting those integrations.

## Runtime model

`WiFiWorker` is implemented with ESPressio Threads `PrecisionThread`. Normal applications configure the manager, initialize/start the worker and do not call `wifi.Poll()` from the application loop. `WiFiManager::ProcessOnce()` remains available as the deterministic single-cycle primitive for tests and specialist integrations, and `Poll()` remains a 0.1.x compatibility alias.

The default worker service period is 50 ms and the default desired execution budget is 5 ms. Explicit operations such as scans, connects and configuration changes bump the worker so they do not need to wait for the next scheduled iteration.

## Remembered Client networks

Client mode persists multiple `ClientNetworkProfile` entries containing SSID, Sensitive password, priority, enabled state and DHCP/static addressing. Automatic selection scans visible networks, ignores disabled/unknown profiles, selects the highest-priority visible remembered profile, uses RSSI for equal-priority ties and advances to the next eligible profile after connection failure when configured.

A healthy current Client connection is intentionally sticky: background/manual scans do not force roaming merely because a higher-priority remembered network appears.

## `APUntilClient`

`WiFiMode::APUntilClient` is intended for devices that normally join an existing network but must remain directly reachable for provisioning/recovery when no usable Client network is available.

At startup WiFi remains STA-only while remembered networks are scanned/attempted. If no usable Client connection is established before the configured fallback timeout, the ESP32 transitions to AP+STA and exposes the configured fallback AP while continuing periodic Client retries. Successful Client connectivity shuts down the fallback AP and returns to STA-only. Subsequent Client loss re-arms the lifecycle. With zero remembered networks the fallback AP starts immediately.

## Callbacks, Observers and Events

WiFi provides direct callback methods and `IWiFiObserver` notifications for mode, Client, AP, scan, station/IP, remembered-network selection and `APUntilClient` lifecycle transitions.

Callbacks are copied and invoked only after WiFi internal state locks are released, allowing callbacks/Observers to safely call back into WiFi.

The optional `WiFiEventBridge` registers itself as a WiFi Observer and converts those synchronous domain observations into Serializable WiFi Events for asynchronous Event subscribers. WiFi owns those Event types because they represent WiFi-domain state.

## Command integration

The optional `WiFiCommandHandler` exposes WiFi-owned controls independently of any particular console/browser transport, including status, mode, scans, remembered-network management, `APUntilClient` controls and configuration save/load. Remembered-network listings never expose plaintext passwords.

WiFi 0.2.1 validates this integration against Command 1.0.3.

## Persistence and Security

The complete `WiFiConfiguration` can be stored through developer-selected ESPressio Persistence providers. WiFi 0.2.1 validates typed persistence against Persistence 0.3.2.

Every remembered-network password is Sensitive/redacted Serializable data. `ProtectedWiFiConfigurationStore` supports authenticated whole-configuration protection through Serializable's protection API and is validated against Serializable 0.11.3, Persistence 0.3.2 and Security 0.4.2. WiFi never chooses the cipher or owns key material.

## Dependencies

```text
WiFi 0.2.1
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.3 < 1.0.0
    -> Threads >= 3.1.7 < 4.0.0

optional integration validation
    - - -> Persistence >= 0.3.2 < 1.0.0
    - - -> Security >= 0.4.2 < 1.0.0
    - - -> Event >= 6.0.3 < 7.0.0
    - - -> Command >= 1.0.3 < 2.0.0
```

Threads is required because autonomous WiFi servicing is core 0.2.x behaviour. Event, Command, Persistence and Security remain opt-in. Serial may consume WiFi, never the reverse. Web infrastructure is intentionally excluded.

## Compatibility

0.2.1 introduces no WiFi public API, configuration-schema, state-machine, callback, Observer, Event Bridge, Command or Persistence runtime changes. Existing WiFi 0.2.0 applications remain source-compatible.

See [CHANGELOG.md](CHANGELOG.md) and `ESPRESSIO_DEPENDENCY_CHART.md` for release history and the coordinated platform position.
