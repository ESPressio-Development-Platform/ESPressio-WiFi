# Persistence and Credential Protection

`WiFiConfiguration` is Serializable, including remembered Client profiles and AP-until-Client timing.

## Ordinary persistence

The optional Persistence integration can store/load the complete configuration through a developer-selected backend.

```cpp
#include <ESPressio_WiFiPersistence.hpp>

WiFiConfigurationStore::Save(storage, "/wifi.espb", config);
WiFiConfigurationStore::Load(storage, "/wifi.espb", config);
```

WiFi does not choose the storage backend.

## Sensitive fields

Client/AP passwords are declared as Sensitive Serializable properties. Redaction prevents accidental inclusion in diagnostic/operator representations; it does **not** encrypt persisted data.

## Protected persistence

For stored credentials, authenticated whole-configuration protection is recommended:

```cpp
#include <ESPressio_WiFiPersistenceSecurity.hpp>
```

The protected store composes Persistence + Serializable protection + Security. WiFi does not choose the cipher or own key material.

Use a purpose/context specific to WiFi configuration so protected bytes cannot be substituted for an unrelated protected record under the same key provider.

## Operator surfaces

Commands or diagnostic listings may report SSID, priority, enabled state and addressing policy but must never return plaintext passwords.

## Extension rule

Keep configuration Serializable and transport/storage neutral. Platform-specific credential stores should be exposed through the Persistence/provider architecture rather than hard-coded into WiFiManager.