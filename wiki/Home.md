# ESPressio WiFi 1.0.0

ESPressio WiFi provides autonomous, platform-neutral WiFi lifecycle and configuration for the ESPressio Development Platform.

It owns Access Point, Client, AP+Client, AP-until-Client and explicit Off modes, remembered Client networks, autonomous selection/failover, scanning, runtime state, Serializable configuration, direct/Observable lifecycle notifications and the platform-neutral `IWiFiPlatform` contract.

Concrete target implementations belong in platform packages such as ESPressio-ESP32.

## Start here

- [Getting Started](Getting-Started)
- [Operating Modes](Operating-Modes)
- [Remembered Client Networks](Remembered-Client-Networks)
- [AP Until Client](AP-Until-Client)
- [Scanning and Selection](Scanning-and-Selection)
- [Radio State and Coordination](Radio-State-and-Coordination)
- [Worker and Runtime Servicing](Worker-and-Runtime-Servicing)
- [Persistence and Credential Protection](Persistence-and-Credentials)
- [Observers, Events and Commands](Observers-Events-and-Commands)
- [Platform Provider Contract](Platform-Provider-Contract)
- [Extending WiFi](Extending-WiFi)
- [API Map](API-Map)

## Architectural boundary

WiFi owns lifecycle and policy; target packages own native WiFi driver calls. Web concerns such as captive portals, HTTP and browser UI belong in ESPressio Web rather than this library.

## Version baseline

This Wiki documents the intended ESPressio **1.0.0** baseline from `feature/20-wifi-off-mode`. Historical pre-1.0 release numbering is intentionally omitted.