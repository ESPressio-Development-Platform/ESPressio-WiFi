# Changelog

## [0.2.0] - 2026-08-23

### Added

- Added Serializable `ClientNetworkProfile` remembered-network entries with per-network SSID, Sensitive password, priority, enabled state and DHCP/static addressing.
- Added persisted `ClientNetworkSelectionConfiguration` controlling automatic selection, startup scanning, disconnect scanning and next-candidate failover.
- Added priority-first automatic selection of visible remembered networks after asynchronous scans.
- Added strongest-RSSI tie breaking for equal-priority profiles and strongest-BSSID selection when multiple APs advertise the same remembered SSID.
- Added automatic fallback to the next eligible remembered profile after a selected network ultimately fails to connect.
- Added explicit `ClientNetworkSelectionState`, candidate and runtime-selection models.
- Added direct callbacks and `IWiFiObserver` notifications for selection lifecycle, selected networks and the no-known-network condition.
- Added Serializable WiFi Event types and Event Bridge support for remembered-network selection.
- Added `wifi client networks` Command controls for listing, adding/updating, reprioritizing and removing remembered profiles without exposing plaintext passwords.
- Added `WiFiWorker`, implemented on ESPressio Threads `PrecisionThread`, for autonomous WiFi servicing.
- Added configurable worker iteration period and desired execution budget.
- Added immediate worker bumping for explicit WiFi work so commands and configuration changes do not wait for the next periodic iteration.
- Added thread-safe snapshot access for configuration, runtime state, scans and remembered-network candidates.
- Added `WiFiMode::APUntilClient` for IoT provisioning/recovery behaviour where the device prefers remembered Client networks but exposes a fallback AP while Client connectivity is unavailable.
- Added Serializable `APUntilClientConfiguration` with configurable fallback timeout and retry-scan interval.
- Added explicit `APUntilClientState` / `APUntilClientRuntimeState` lifecycle reporting for seeking, fallback-AP and connected phases.
- Added `WiFiManager::OnAPUntilClientStateChanged(...)` and `IWiFiObserver::OnAPUntilClientStateChanged(...)` notifications.
- Added Serializable `WiFiAPUntilClientStateChangedEvent` and WiFi Event Bridge forwarding for asynchronous lifecycle subscribers.
- Added immediate fallback AP activation when no remembered networks exist.
- Added delayed fallback when remembered networks exist but no usable Client connection can be established before the configured timeout.
- Added AP+STA retry behaviour while the fallback AP is active, allowing remembered networks to be retried without deliberately dropping provisioning/control clients.
- Added automatic fallback-AP shutdown and return to STA-only radio mode after successful Client connectivity.
- Added automatic re-arming of the fallback lifecycle if an established Client connection is subsequently lost.
- Added immediate remembered-network retry when a profile is added or updated during `APUntilClient` fallback operation.
- Added `wifi mode ap-until-client`, `wifi ap-until-client status`, `wifi ap-until-client retry-now`, `wifi ap-until-client fallback-timeout` and `wifi ap-until-client retry-interval` Command controls.

### Changed

- WiFi now requires ESPressio Serializable `>=0.11.2 <1.0.0` and ESPressio Threads `>=3.1.6 <4.0.0` for the corrected Serializable 0.11.2 cascade generation.
- Optional integration validation now targets Persistence `>=0.3.1 <1.0.0`, Security `>=0.4.1 <1.0.0`, Event `>=6.0.2 <7.0.0`, and Command `>=1.0.2 <2.0.0`.
- ESP32 integration CI validates the exact released coordinated generation including Units 0.2.6 and Timing 2.2.7.
- Normal applications no longer call `wifi.Poll()` from `loop()`; `WiFiWorker` services the manager automatically.
- `WiFiManager::ProcessOnce()` is the deterministic single-cycle runtime primitive used by the worker and host tests.
- `Poll()` remains as a 0.1.x compatibility alias.
- The ESP32 platform adapter now receives a selected `ClientNetworkProfile`; remembered-network choice remains platform-neutral manager logic.
- A healthy current Client connection is sticky by default: manual/background scans do not force roaming merely because a higher-priority remembered network appears.
- Callbacks are copied and invoked outside internal WiFi state locks so notifications may safely call back into WiFi.
- The ESP32 adapter now transitions `APUntilClient` between STA-only and AP+STA at runtime rather than treating it as permanent AP+Client operation.

### Security

- Every remembered-network password is Sensitive/redacted Serializable data.
- Remembered-network Command output never exposes stored passwords.
- Complete remembered profile collections continue to support optional whole-configuration authenticated protection through Persistence 0.3.1 / Serializable 0.11.2 / Security 0.4.1.

### Compatibility

- Existing 0.1.x single-client SSID/password/addressing fields and commands remain available for source compatibility.
- `Poll()` remains available for existing manually serviced applications, though new code should use `WiFiWorker`.
- `AccessPointClient` remains a persistent AP+Client mode; the new `APUntilClient` behaviour is opt-in and does not alter existing AP+Client semantics.
- This is a backward-compatible public feature release and advances WiFi to 0.2.0.

### Tracking

- Implements #11 — preferred persisted Client networks and automatic selection.
- Implements #12 — autonomous WiFi runtime on ESPressio PrecisionThread.
- Implements #15 — `APUntilClient` IoT fallback/provisioning mode.
- Incorporates #16 — corrected Serializable 0.11.2 dependency cascade.

## [0.1.0] - 2026-08-23

### Added

- First production-oriented ESPressio WiFi API.
- Platform-neutral WiFi, IPv4, MAC, security, scan and runtime-state models.
- Independent Client and Access Point runtime state machines plus a composite snapshot.
- Access Point, Client, AP+Client and Disabled operating modes.
- Always-Serializable WiFi configuration with sensitive credential metadata.
- Serializable DHCP/static client addressing and AP network/DHCP-server configuration.
- Reconnect policy, hostname, channel, transmit-power and power-save configuration.
- Compile-time extensible configuration envelope through `WiFiConfigurationWith<TExtension>` and `ConfiguredWiFiManager<TConfiguration>`.
- Hardware-neutral `IWiFiPlatform` and concrete ESP32 Arduino implementation.
- Asynchronous network scanning with ESPressio-owned results.
- Direct callbacks for logical AP/Client/mode transitions and important AP/client lifecycle events.
- `IWiFiObserver` / Observable notifications for mode, Client, AP, scan, station and IP lifecycle changes.
- Optional WiFi-owned Event types and `WiFiEventBridge`.
- Optional `WiFiCommandHandler` for Serial/browser-independent command control.
- Optional Persistence configuration store for file and key/value providers.
- Optional protected Persistence/Security configuration store protecting the complete Serializable document.
- Host manager/observer tests, ESP32 integration compile validation and adoption-focused documentation.

### Security

- Password fields are declared Sensitive for diagnostic redaction.
- Commands permit setting credentials but deliberately provide no plaintext credential-read command.
- Documentation strongly recommends protected persistence for configurations containing credentials.

### Compatibility

- This is the first functional release. The historical manifest-only `1.0.0` value did not represent a released public implementation and creates no compatibility obligation.
