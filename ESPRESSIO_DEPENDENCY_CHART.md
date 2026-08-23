# ESPressio WiFi 0.1.0 dependency position

Arrows point from WiFi to the library it consumes.

```text
WiFi 0.1.0
    -> Serializable >= 0.11.0 < 1.0.0

Optional integrations
    - - -> Persistence >= 0.3.0 < 1.0.0
    - - -> Security >= 0.4.0 < 1.0.0
            indirectly through protected Serializable Persistence
    - - -> Observable >= 3.0.1 < 4.0.0
    - - -> Event >= 6.0.1 < 7.0.0
    - - -> Command >= 1.0.1 < 2.0.0
```

Serializable is required because `WiFiConfiguration` is deliberately a Serializable model. Credentials are declared sensitive so diagnostic/redacted serialization does not expose them.

Persistence is optional. `ESPressio_WiFiPersistence.hpp` persists the complete configuration through a developer-supplied `IFileStorage` or `IKeyValueStorage`.

Security remains optional. `ESPressio_WiFiPersistenceSecurity.hpp` consumes the protected Persistence/Serializable surface; WiFi does not select cryptographic algorithms or manage keys.

Observable, Event and Command are integration boundaries layered above the platform-neutral state model. They must not leak platform-specific WiFi types into their APIs.

Serial is terminal/downstream:

```text
Serial - - -> WiFi
WiFi -> Serial   NONE
```

Web/HTTP/WebSocket infrastructure is intentionally not part of WiFi.
