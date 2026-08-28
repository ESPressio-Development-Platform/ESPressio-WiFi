# Extending ESPressio WiFi

Extensions should preserve WiFi's role as the platform-neutral owner of WiFi lifecycle and policy.

## New operating modes

A new mode must define coherent Client/AP/radio lifecycle semantics, runtime-state representation, configuration behaviour and platform-provider requirements. Do not add a mode that is merely a target-specific driver flag.

## New selection/retry policy

Keep remembered-network selection deterministic and testable. Avoid introducing hidden roaming or reconnection behaviour that contradicts the current sticky-healthy-connection rule unless that becomes an explicit policy choice.

## New runtime work

Integrate periodic/asynchronous servicing with the existing `WiFiWorker` rather than creating unrelated native tasks. Preserve explicit worker cadence and bump/wake behaviour.

## New platform targets

Implement `IWiFiPlatform` in the target/platform package. Keep native WiFi types and SDK calls below that boundary.

## Shared-radio consumers

Use `IWiFiRadioObserver` and authoritative `WiFiRadioState` for low-level coordination. Do not require downstream radio consumers to poll or independently infer native channel/interface state.

## New operator or Web features

WiFi owns remembered networks and lifecycle. Serial/Web integrations should call the existing WiFiManager API rather than duplicating configuration databases or connection state machines.

HTTP, captive portal and browser UI remain Web concerns.

## Memory and locking

Return snapshots rather than exposing mutable internal containers. Invoke callbacks after internal state locks are released. Preserve bounded/deterministic servicing on embedded targets.

## Testing expectations

Cover all mode transitions, Off semantics, remembered-network ordering/ties, connection stickiness, failure fallback, APUntilClient timing, scans, concurrent API calls, callback re-entry, worker bumping, persistence migration, credential redaction/protection and radio-state accuracy.