# Radio State and Coordination

`WiFiRuntimeState` describes application-facing WiFi lifecycle. `WiFiRadioState` separately describes the authoritative physical state of the shared 2.4 GHz radio.

The distinction matters for low-level consumers such as ESP-NOW.

## `WiFiRadioState`

The radio snapshot includes:

- physical radio mode (`Off`, `Station`, `AccessPoint`, `AccessPointStation`);
- whether STA/AP interfaces are active;
- whether STA is connected;
- whether a scan is active;
- current effective channel;
- STA and AP MAC addresses.

## Radio observer

`IWiFiRadioObserver` is intended for infrastructure consumers that need immediate authoritative coordination with radio transitions. It is separate from normal application-oriented WiFi lifecycle observation.

For example, ESP-Now uses this surface to suspend sends during scans/transitions and reconcile peer interfaces after AP/STA/APSTA changes.

## Why application Events are not sufficient

Shared-radio coordination is part of the low-level transition itself. It should not depend on asynchronous Event delivery occurring later in another execution context.

Application Events can still mirror lifecycle information for normal application subscribers.

## Off state

`WiFiMode::Off` should result in a radio snapshot with `WiFiRadioMode::Off` and inactive interfaces once the transition completes.

## Extension contract

A platform/provider must publish snapshots that reflect actual native radio facts. Consumers depend on this state for correctness, so do not infer a convenient application state when the native interface/channel differs.

Keep physical radio authority in WiFi rather than allowing downstream shared-radio libraries to independently change native WiFi mode/channel.