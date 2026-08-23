# ESPressio WiFi 0.1.0 dependency position

Arrows point from the consuming library to the library it consumes. Solid edges are required; dashed edges are opt-in integrations.

```text
WiFi 0.1.0
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.0 < 1.0.0

WiFi persistence integration
    - - -> Persistence >= 0.3.0 < 1.0.0

WiFi protected persistence integration
    - - -> Persistence >= 0.3.0 < 1.0.0
    - - -> Serializable >= 0.11.0 < 1.0.0
    - - -> Security >= 0.4.0 < 1.0.0

WiFi Event bridge
    - - -> Event >= 6.0.1 < 7.0.0

WiFi Command handler
    - - -> Command >= 1.0.1 < 2.0.0

Serial 0.8.x
    - - -> WiFi >= 0.1.0 < 1.0.0
```

Observable is part of the normal WiFi contract because externally meaningful WiFi lifecycle transitions are observable. Serializable is also foundational because every standard WiFi configuration is Serializable.

Persistence remains developer-selected and backend-neutral. WiFi does not depend on LittleFS, NVS, SD, or any other concrete persistence mechanism.

Security remains optional and enters only when the developer selects protected configuration persistence. WiFi never chooses an algorithm or owns keys.

Event and Command integrations are owned by WiFi to preserve dependency direction. Event and Command themselves do not learn about WiFi.

Serial is terminal/downstream: `Serial - - -> WiFi`; `WiFi -> Serial` never exists.

Web/HTTP/WebSocket/browser infrastructure is intentionally outside ESPressio WiFi and will belong to ESPressio Web.
