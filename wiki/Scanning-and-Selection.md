# Scanning and Selection

Scanning is asynchronous and serviced by `WiFiWorker`; normal applications do not poll for completion manually.

## Automatic selection

`ClientNetworkSelectionConfiguration` controls:

```text
AutomaticSelection
ScanOnStartup
ScanOnDisconnect
TryNextOnFailure
```

The default policy scans when Client operation starts, matches visible SSIDs to remembered profiles and selects deterministically by priority then signal strength.

## Duplicate SSIDs

When several BSSIDs advertise the same remembered SSID, the strongest visible candidate is preferred.

## Manual scans

Applications can request a scan and observe completion/results without changing the healthy current connection. A manual/background scan is discovery, not an instruction to roam immediately.

## Shared-radio effect

A scan temporarily hops the physical radio through channels. WiFi publishes this fact through its radio lifecycle/snapshot so shared-radio consumers such as ESP-Now can suspend incompatible transmission during the scan.

## Snapshot APIs

Configuration, runtime state, last scan results and eligible-network queries return snapshots rather than mutable references into WiFiManager internals.

## Callback safety

Callbacks/Observers are invoked after WiFi internal state locks are released, allowing observers to call back into WiFi without running beneath the manager state mutex.