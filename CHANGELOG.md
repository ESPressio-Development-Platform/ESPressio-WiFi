# Changelog

## [0.2.1] - 2026-08-24

### Added

- Added platform-neutral `WiFiRadioState` / `WiFiRadioMode` snapshots exposing the authoritative shared-radio facts required by infrastructure consumers: active STA/AP interfaces, STA connectivity, scan activity, current primary channel and the STA/AP MAC addresses.
- Added `IWiFiRadioObserver`, a dedicated low-level infrastructure Observer contract kept separate from the existing application-level `IWiFiObserver` API.
- Added `WiFiManager::RadioState()`, `RegisterRadioObserver()` and `UnregisterRadioObserver()`.
- Added deterministic radio-transition beginning/completed callbacks around configuration, Client connect/disconnect, Access Point start/stop and automatic remembered-network connection operations.
- Added explicit WiFi scan beginning/completed radio lifecycle callbacks so shared-radio consumers can quiesce while the ESP32 radio is channel-hopping.
- Added spontaneous authoritative radio-state change publication from normal worker polling so infrastructure-driven STA/channel changes are visible even when they were not initiated by a direct WiFiManager operation.
- Added ESP32 radio-state reporting from the native driver through `esp_wifi_get_mode()`, `esp_wifi_get_channel()` and the STA/AP interface MACs.
- Added host regression coverage for the low-level shared-radio lifecycle independently from normal WiFi Observer behavior.

### Changed

- Raised required ESPressio Serializable to `>=0.11.3 <1.0.0`.
- Raised required ESPressio Threads to `>=3.1.7 <4.0.0` while preserving Observable `>=3.0.2 <4.0.0`.
- Updated ESP32 integration validation to released Serializable 0.11.3, Units 0.2.7, Timing 2.2.8, Threads 3.1.7, Persistence 0.3.2, Security 0.4.2, Event 6.0.3 and Command 1.0.3.
- Removed the temporary Serializable bugfix-branch pin from CI; all ESPressio integration dependencies now use released tags.
- Updated PlatformIO, Arduino and component version metadata for WiFi 0.2.1.
- WiFi is now explicitly the authority for shared ESP32 radio mode/channel state. Low-level consumers such as ESP-NOW can coordinate against WiFi synchronously without routing radio-lifecycle control through application Events.

### Architecture

- `IWiFiObserver` remains the application-semantic lifecycle surface and is source-compatible.
- `IWiFiRadioObserver` is an additive infrastructure surface for components that physically share the WiFi radio.
- WiFi does not depend on ESP-NOW. The dependency direction remains one-way: optional ESP-NOW integration observes WiFi's authoritative radio lifecycle.

### Compatibility

- Existing WiFi configuration schema, Commands, Event Bridge, Persistence behavior and application-level Observer callbacks remain source-compatible.
- The shared-radio lifecycle API is additive.
- Version metadata intentionally remains **0.2.1** for this in-place correction/development cycle; no new release is introduced by issue #21.

### Tracking

- Closes #17.
- Implements #21 — coordinated radio lifecycle for shared-radio consumers such as ESP-NOW.

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
