# Platform Provider Contract

`IWiFiPlatform` is the target-neutral boundary between ESPressio WiFi lifecycle/policy and the concrete platform driver.

WiFiManager owns state machines, selection policy, configuration, retries, subscriptions and application semantics. The platform provider maps those requests to the target WiFi implementation and reports authoritative runtime/radio facts back upward.

## Provider responsibilities

A provider must implement the platform operations required to:

- place the radio into Off/STA/AP/AP+STA operation;
- start/stop configured AP operation;
- connect/disconnect the Client interface;
- start and collect asynchronous scans;
- apply DHCP/static network configuration;
- apply hostname, transmit power and power-save settings where supported;
- report current channel and interface MAC addresses;
- expose native lifecycle changes in ESPressio-owned WiFi types.

## Public-type boundary

Arduino/ESP-IDF WiFi enums, events, handles and address structures must not cross `IWiFiPlatform`. Translate them into ESPressio `WiFiMode`, `WiFiRadioMode`, `IPv4Address`, `MacAddress`, `ScanResult` and related types.

## Authoritative radio snapshots

The provider is responsible for accurate `WiFiRadioState` facts. Shared-radio consumers depend on these values for correctness.

Do not report the configured application mode if the physical driver is temporarily scanning, transitioning, or using a different active interface/channel.

## Off semantics

`WiFiMode::Off` is the canonical explicit radio-off request. The provider must actually stop WiFi radio operation according to the target's supported mechanism and publish the resulting radio-off state.

Legacy `Disabled` has equivalent platform effect but should not become a separate native behaviour.

## Asynchronous scanning

Scanning is modelled asynchronously. Do not turn the provider contract into a long blocking call that prevents the WiFi worker from servicing lifecycle state.

## Error handling

Native errors should be translated into stable WiFi-domain outcomes while retaining sufficient diagnostic information inside the platform implementation where useful. Higher-level code should not need to interpret `esp_err_t` or other target result types.

## Testing

Provider conformance should cover Off/STA/AP/AP+STA transitions, scan start/completion/failure, connection/disconnection, DHCP/static addressing, channel reporting, MAC reporting, AP station counts, configuration changes, and accurate radio-state snapshots across transitions.