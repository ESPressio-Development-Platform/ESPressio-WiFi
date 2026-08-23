# Changelog

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
