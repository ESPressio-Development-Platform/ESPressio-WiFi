# Operating Modes

`WiFiMode` defines the high-level lifecycle policy:

```text
Off
Client
AccessPoint
AccessPointClient
APUntilClient
```

`Disabled` remains a legacy compatibility value with the same platform effect as `Off`; new code and Commands should use `Off`.

## Off

`Off` is the canonical explicit radio-off mode. Both STA and AP operation are disabled and the platform provider is expected to place the WiFi radio into its off state.

## Client

Client mode joins remembered infrastructure networks using the configured selection/reconnect policy.

## Access Point

Access Point mode exposes the configured SoftAP continuously.

## AccessPointClient

AP and Client remain active together. The AP does not disappear merely because Client connectivity succeeds.

## APUntilClient

STA connectivity is preferred. The fallback AP appears only while Client connectivity is unavailable and is removed when Client connectivity succeeds.

See [AP Until Client](AP-Until-Client).

## Runtime state

`WiFiRuntimeState` reports Client, AP, scan and AP-until-Client application state separately from the physical shared-radio snapshot exposed through `WiFiRadioState`.