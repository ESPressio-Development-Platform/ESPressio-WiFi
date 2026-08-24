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
