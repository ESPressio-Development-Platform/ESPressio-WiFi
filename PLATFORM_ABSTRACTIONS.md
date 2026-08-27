# Platform Abstractions Audit Trail

This file records changes made during the platform-abstraction tranche tracked by issue #28.

## 2026-08-27

- Retained `IWiFiPlatform`, WiFi configuration/state types, lifecycle management, reconnect policy, scanning, observer/event integration and persistence integration in ESPressio-WiFi.
- Removed `ESPressio_ESP32WiFi.hpp` from this repository after relocating the concrete implementation to ESPressio-ESP32.
- Removed `ESPressio_ESP32WiFiRadio.hpp` after relocating the ESP32-specific RF diagnostics/policy helpers to ESPressio-ESP32.
- The relocated ESP32 implementation now consumes `System::Clock::Monotonic()` for reconnect/connection timeout scheduling instead of Arduino `millis()`.
- ESPressio-WiFi does not depend on ESPressio-ESP32; the dependency direction is intentionally inverted so platform packages implement the WiFi-owned contract.

## Boundary

ESPressio-WiFi owns the WiFi domain and the `IWiFiPlatform` contract. ESPressio-ESP32 owns the Arduino/ESP-IDF concrete implementation. Applications select/install a concrete WiFi platform at composition time.
