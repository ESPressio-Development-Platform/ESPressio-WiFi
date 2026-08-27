# ESPressio WiFi Optimisation Log

This file records the current resource-optimisation round chronologically. Version numbers are intentionally unchanged during this round.

Rollback anchor: `rollback/optimisations-pre-20260825` -> `69eda04fd49d12067d5fcdcecb69a320ee4854a9`.

## 2026-08-25 — Optimisation tracking opened (#21)

### Context
WiFi already has active shared-radio lifecycle work on `feature/20-wifi-off-mode`. The optimisation round does not currently change WiFi runtime behavior; this repository is participating because it explicitly consumes ESPressio Threads and must exercise the active Threads optimisation branch during integration testing.

### Dependency pin
`library.json` resolves ESPressio Threads directly from `feature/73-memory-efficiency` on the working branch.

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
WiFi remains the authoritative radio owner whenever ESPressio WiFi is present. Disruptive radio changes continue to publish low-level transition callbacks before and after the platform operation. ESP-Now #44 consumes those callbacks transactionally: new sends are blocked and native ESP-NOW is detached before the driver transition, then rebuilt/reconciled against the resulting WiFi state afterward.

### Safety / rollback
The new helper is additive and does not change normal WiFi runtime behavior by itself. Existing `TxPowerDbm` and `PowerSave` configuration semantics remain unchanged. The pre-optimisation rollback branch remains the repository-level recovery point.

### Commits
- `a918417` — `feat(#22): add canonical ESP32 radio fingerprint and policy`

## 2026-08-25 — Pin Serializable PSRAM-buffer optimisation (#22 / Serializable #25)

WiFi explicitly consumes ESPressio Serializable for configuration/state serialization. During this hardware-optimisation round, `library.json` resolves Serializable directly from `optimisation/25-psram-buffers` so WiFi integration tests cannot silently use the released allocator behavior while the Lab validates external-RAM-backed serialization.

The dependency change does not itself enable the opt-in PSRAM policy; applications choose that with `ESPRESSIO_SERIALIZATION_PREFER_PSRAM`. Boards without PSRAM remain safe because the prefer-PSRAM allocator falls back to internal 8-bit memory.

Commit: `128ef65610fcfe3e2d12996866d977b0c0517452`.

## 2026-08-26 — Compact retained ESP32 platform configuration (#23)

### Hardware context
Post-ESP-NOW stack/lifetime optimisation moved the current Lab failure away from worker stack canaries and into native WiFi SoftAP/internal-heap startup. The useful next step is therefore to maximize internal-memory headroom at the native SoftAP boundary without changing ESP-IDF radio behavior.

### Finding
`WiFiManager::Configure()` owns the complete `WiFiConfiguration`, but `ESP32WiFiPlatform::Apply()` previously copied and retained a second complete configuration before calling `WiFi.softAP()`. That duplicate included the manager-owned remembered-client `Networks` vector, its SSID/password strings, client-selection policy, APUntilClient policy and hostname even though the platform does not require those fields after `Apply()`.

### Change
`ESP32WiFiPlatform` now retains a private compact operational snapshot containing only the state needed by its native runtime paths:
- operating mode;
- legacy client enabled/SSID/password/addressing/static network;
- access-point configuration and DHCP settings;
- reconnect policy;
- TX power and power-save policy.

Automatic remembered-profile selection is still derived synchronously from the caller's complete configuration during `Apply()`. `ConnectClient(profile)`, AP start/stop, DHCP, reconnect and radio-policy behavior remain unchanged. The platform no longer retains the remembered-network collection, selection policy, APUntilClient policy or hostname.

### Expected memory effect
This removes one long-lived duplicate `std::vector<ClientNetworkProfile>` plus its profile/string allocations from the ESP32 platform before native SoftAP startup. It does not change or hide ESP-IDF's own internal-memory requirements; it simply returns ESPressio-owned headroom to the allocator at the point where native WiFi needs it.

### Integration graph
Working-branch CI now validates WiFi against the active memory-efficiency heads rather than released/stale dependencies:
- Observable `feature/16-rtti-free-observer-registry`;
- Serializable `optimisation/25-psram-buffers`, with `ESPRESSIO_SERIALIZATION_PREFER_PSRAM` enabled;
- Threads `feature/73-memory-efficiency`;
- Event `feature/57-rtti-free-memory-efficiency`;
- Command `feature/32-memory-efficiency`.

The ESP32 integration consumer also transfers its test configuration into `WiFiManager::Configure()` with `std::move`, exercising the API's intended ownership-transfer path rather than retaining an avoidable caller-side copy during setup.

### Safety / validation
The existing ESP32 integration consumer configures `APUntilClient` with a remembered preferred network, so CI continues to compile the automatic-profile path against the compact retained state. Hardware free/largest-internal-heap telemetry remains the authority for measuring the actual SoftAP headroom improvement.

### Commits
- `86af5eb` — `optimise(#23): retain compact ESP32 WiFi platform configuration`
- `3da55c3` — `chore(#23): validate against active Threads optimisation branch`
- `e25717f` — `test(#23): exercise active memory-efficiency branches`

## 2026-08-27 — System-backed WiFi metadata and reduced configuration copying (#24)

Phase 9 of the coordinated memory-policy programme moves eligible ESPressio-owned dynamic WiFi bookkeeping to the System abstraction without changing the Serializable public configuration schema.

### Changes
- retained scan-result and eligible-candidate storage now uses ESPressio-System `ExternalPreferred` containers;
- manager and radio Observable allocations use the System external-preferred allocator;
- added `WithConfiguration()` for synchronous lock-scoped read access where a caller does not require an independent configuration snapshot;
- candidate construction reads remembered profiles in place under the manager lock instead of first copying the complete `WiFiConfiguration`;
- recurring APUntilClient/selection paths now snapshot only the scalar policy values they need instead of copying the complete configuration and remembered-network collection;
- completed candidate lists are moved into retained storage rather than copied;
- the candidate-empty decision is captured under the manager lock, avoiding an unlocked read of retained candidate storage;
- independent snapshots crossing into storage/platform/user ownership remain copies deliberately.

### Schema boundary
The public Serializable `WiFiConfiguration` and its `std::string`/`std::vector` member types remain unchanged in this phase. Changing those types would be a serialization/API migration rather than a memory-placement implementation detail. The manager instead reduces duplicate ownership and uses System-backed storage for its independent dynamic result/candidate metadata.

### Dependency policy
The current working manifest resolves Threads from `optimisation/69-resource-footprint` and ESPressio-System from `feature/1-system-memory-policy`; no release version number changed.

### Commits
- `26df43b` — main manager storage/copy-reduction implementation
- `c3e23d3` — working-branch System dependency metadata
- `f522e7a` — lock-boundary correction for candidate empty-state inspection
