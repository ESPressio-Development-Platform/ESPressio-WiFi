# Remembered Client Networks

Client mode can store multiple remembered network profiles. Each profile owns its SSID, sensitive password, priority, enabled state and DHCP/static addressing configuration.

```cpp
ClientNetworkProfile home;
home.SSID = "Home";
home.Password = "home-password";
home.Priority = 300;

config.Client.Networks = { home };
```

## Selection order

With automatic selection enabled, WiFi scans visible networks, ignores disabled/unknown profiles, chooses the highest-priority visible profile, uses strongest RSSI to break equal-priority ties, and chooses the strongest BSSID for duplicate SSIDs.

If `TryNextOnFailure` is enabled, a failed candidate advances to the next eligible remembered profile.

## Sticky healthy connections

A healthy current Client connection is intentionally sticky. A later scan does not disconnect a working network merely because a higher-priority remembered network becomes visible.

## Addressing

Each profile can use DHCP or its own static network configuration. This allows one device to remember networks with different addressing requirements.

## Runtime management

Profiles can be added, updated, reprioritized or removed through the WiFiManager API. Persist configuration explicitly when changes should survive reboot.

## Credential handling

Passwords are Sensitive Serializable fields. Operator listings and diagnostics must not reveal them. For stored credentials, use protected persistence rather than relying on redaction.