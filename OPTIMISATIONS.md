# ESPressio WiFi Optimisation Log

This file records the current resource-optimisation round chronologically. Version numbers are intentionally unchanged during this round.

Rollback anchor: `rollback/optimisations-pre-20260825` -> `69eda04fd49d12067d5fcdcecb69a320ee4854a9`.

## 2026-08-25 — Optimisation tracking opened (#21)

### Context
WiFi already has active shared-radio lifecycle work on `feature/20-wifi-off-mode`. The optimisation round does not currently change WiFi runtime behavior; this repository is participating because it explicitly consumes ESPressio Threads and must exercise the active Threads optimisation branch during integration testing.

### Dependency pin
`library.json` now resolves ESPressio Threads directly from `optimisation/69-resource-footprint` on the working branch.

### Existing resource-related design
- WiFi remains authoritative for native radio mode/channel.
- ESP-NOW coordination is direct and lifecycle-driven rather than Event-mediated.
- No WiFi version bump is made; 0.2.0 remains the mutable working release.

### Safety / rollback
No new WiFi runtime optimisation is accepted without its own issue/rationale. The rollback branch above is the pre-optimisation working-head reference.

## 2026-08-25 — Canonical shared-radio fingerprint and policy (#22)

### Hardware evidence
The Lab's ESP-NOW-only control remains highly reliable when it establishes a stable AP radio first and ESP-NOW attaches without owning WiFi initialization. ESPressio WiFi + ESP-NOW remains unreliable even when the visible mode/channel are also AP/channel 1, proving that comparing only `wifi_mode_t` and primary channel is insufficient. Explicit AP→Client testing additionally produced ESP-NOW interface mismatch errors, and Client→AP produced a native ESP-IDF SoftAP startup crash while ESP-NOW remained attached.

### Changes
- Added `ESPressio_ESP32WiFiRadio.hpp` with `ESP32WiFiRadioFingerprint` so tests and shared-radio consumers can compare RF-relevant native state directly: mode, primary/secondary channel, power-save mode and configured maximum TX power.
- Added `ReadESP32WiFiRadioFingerprint()` for low-cost native-state inspection.
- Added `ApplyESP32WiFiRadioPolicy()` implementing the same bounded TX-power and explicit power-save semantics used by ESPressio WiFi configuration, allowing the standalone ESP-NOW Lab path to establish an equivalent RF baseline.
- IP configuration, DHCP, hostname and station bookkeeping remain intentionally outside the fingerprint because they are network-layer concerns, not RF equivalence.

### Coordination contract
WiFi remains the authoritative radio owner whenever ESPressio WiFi is present. Disruptive radio changes continue to publish low-level transition callbacks before and after the platform operation. ESP-Now #44 now consumes those callbacks transactionally: new sends are blocked and native ESP-NOW is detached before the driver transition, then rebuilt/reconciled against the resulting WiFi state afterward.

### Safety / rollback
The new helper is additive and does not change normal WiFi runtime behavior by itself. Existing `TxPowerDbm` and `PowerSave` configuration semantics remain unchanged. The pre-optimisation rollback branch remains the repository-level recovery point.

### Commits
- `a918417` — `feat(#22): add canonical ESP32 radio fingerprint and policy`
