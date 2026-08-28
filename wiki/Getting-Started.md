# Getting Started

ESPressio WiFi separates portable WiFi lifecycle/policy from the target-specific implementation.

A typical composition creates the target provider, a `WiFiManager`, and the autonomous `WiFiWorker`:

```cpp
#include <ESPressio_WiFi.hpp>
#include <ESPressio_WiFiWorker.hpp>
#include <ESPressio_WiFiPlatform.hpp>

using namespace ESPressio::WiFi;

WiFiPlatform platform;
WiFiManager wifi(platform);
WiFiWorker worker(wifi);
```

Configure the desired mode before starting the worker:

```cpp
WiFiConfiguration config;
config.Mode = WiFiMode::AccessPoint;
config.AccessPoint.SSID = "ESPressio-Device";
config.AccessPoint.Password = "change-me";

wifi.Configure(config);
worker.Initialize();
worker.Start();
```

Normal applications do not need to call `Poll()` from `loop()`.

## Platform-neutral API

Public WiFi types are ESPressio-owned IPv4/MAC/network/security/scan types. Arduino/ESP-IDF WiFi types remain below `IWiFiPlatform`.

## Configuration is Serializable

`WiFiConfiguration` and its nested configuration types are Serializable. Credentials are marked Sensitive/redacted, but redaction is not encryption; use protected Persistence when stored credentials require confidentiality/authentication.

## Next steps

Choose an [Operating Mode](Operating-Modes), then read [Remembered Client Networks](Remembered-Client-Networks) and [Worker and Runtime Servicing](Worker-and-Runtime-Servicing).