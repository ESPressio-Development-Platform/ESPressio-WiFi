# AP Until Client

`WiFiMode::APUntilClient` is an IoT fallback mode for devices that should normally join infrastructure WiFi but remain directly reachable while Client connectivity is unavailable.

## Lifecycle

```text
remembered networks exist
        |
        v
   STA-only startup
        |
        +---- Client connects ----------------------> STA-only
        |
        +---- fallback timeout expires
                         |
                         v
                    AP + STA
                    fallback AP
                         |
                         +---- periodic scan/retry
                         |
                         +---- Client connects ------> stop AP -> STA-only
```

If there are no remembered networks, the fallback AP starts immediately because there is nothing useful for STA to attempt.

## Timing

`FallbackTimeoutMilliseconds` controls how long STA-only connection attempts are allowed before the fallback AP appears.

`RetryScanIntervalMilliseconds` controls retry cadence while fallback is active.

The fallback AP remains available during retries and disappears only after Client connectivity is actually established.

## Provisioning

Adding/updating a remembered network while fallback is active can trigger an immediate connection attempt. A Serial console, application code or future Web provisioning UI can therefore all use the same WiFi-owned remembered-network API.

## Difference from permanent AP+Client

`AccessPointClient` keeps the AP active continuously. `APUntilClient` exists specifically to remove the AP after successful Client connectivity and restore it later if connectivity is lost long enough to require fallback again.