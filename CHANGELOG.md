# Changelog

## [0.1.0] - 2026-08-23

### Added

- First production-oriented ESPressio WiFi API.
- Platform-neutral WiFi, IP, MAC, security, scan and runtime-state models.
- Independent Client and Access Point state models plus composite runtime state.
- Serializable WiFi configuration with sensitive credential fields.
- Access Point, Client, AP+Client and Disabled operating modes.
- DHCP/static client addressing and AP DHCP-server configuration model.
- Reconnect policy, hostname, channel, TX-power and power-save configuration.
- Hardware-neutral `IWiFiPlatform` and `WiFiManager` lifecycle surface.
- Asynchronous scan initiation/completion model.
- Optional Persistence configuration store and protected Persistence/Security store.
- Adoption-focused README and dependency documentation.

### Compatibility

- This is the first functional release. The historical manifest-only `1.0.0` value did not represent a released public implementation and creates no compatibility obligation.
