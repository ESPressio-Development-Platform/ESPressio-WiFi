# API Map

## Core service

- `WiFiManager` — authoritative WiFi lifecycle/configuration service.
- `WiFiWorker` / `WiFiWorkerConfiguration` — autonomous precision-thread runtime servicing.
- `IWiFiPlatform` — target/platform implementation contract.

## Configuration

- `WiFiConfiguration` — complete Serializable configuration.
- `ClientConfiguration` — Client settings and remembered network profiles.
- `ClientNetworkProfile` — SSID, sensitive password, priority, enabled state and addressing.
- `ClientNetworkSelectionConfiguration` — automatic selection/scan/failover policy.
- `AccessPointConfiguration` — AP identity/channel/network/DHCP settings.
- `ReconnectPolicy` — Client retry/backoff settings.
- `APUntilClientConfiguration` — fallback/retry timing.

## Runtime types

- `WiFiMode` — Off, Client, AccessPoint, AccessPointClient, APUntilClient plus legacy Disabled compatibility value.
- `WiFiRuntimeState` — application-facing Client/AP/scan/fallback state.
- `WiFiRadioState` — authoritative physical shared-radio facts.
- `ScanResult`, `ClientNetworkCandidate` and ESPressio-owned network/address/security types.

## Observation

- `IWiFiObserver` — synchronous application lifecycle observation.
- `IWiFiRadioObserver` — synchronous infrastructure/shared-radio observation.

## Optional integrations

- WiFi Persistence store.
- protected WiFi Persistence/Security store.
- WiFi Event bridge and Event types.
- WiFi Command handler.

## Dependency direction

WiFi core consumes System, Observable, Serializable and Threads. Platform implementations live downstream in platform packages such as ESPressio-ESP32. Persistence/Security/Event/Command integrations remain optional.

Web interfaces are downstream consumers of WiFi lifecycle/configuration, not part of WiFi itself.