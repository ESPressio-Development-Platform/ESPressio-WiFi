# ESPressio WiFi 0.2.0 dependency position

Arrows point from the consuming library to the library it consumes. Solid edges are required; dashed edges are opt-in integrations.

```text
WiFi 0.2.0
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.2 < 1.0.0
    -> Threads >= 3.1.6 < 4.0.0

WiFi persistence integration
    - - -> Persistence >= 0.3.1 < 1.0.0

WiFi protected persistence integration
    - - -> Persistence >= 0.3.1 < 1.0.0
    - - -> Serializable >= 0.11.2 < 1.0.0
    - - -> Security >= 0.4.1 < 1.0.0

WiFi Event bridge
    - - -> Event >= 6.0.2 < 7.0.0

WiFi Command handler
    - - -> Command >= 1.0.2 < 2.0.0

Serial 0.8.x
    - - -> WiFi >= 0.2.0 < 1.0.0
```

The coordinated released generation validated by WiFi 0.2.0 is:

```text
Observable    3.0.2
Serializable  0.11.2
Units         0.2.6
Timing        2.2.7
Threads       3.1.6
Event         6.0.2
Command       1.0.2
Security      0.4.1
Persistence   0.3.1
Sockets       0.7.2
ESP-Now       0.8.2
WiFi          0.2.0
```

Observable is part of the normal WiFi contract because externally meaningful WiFi lifecycle and remembered-network selection transitions are observable.

Serializable is foundational because WiFi configuration—including all remembered Client profiles, credentials, priorities and addressing—is Serializable.

Threads is required because normal WiFi runtime servicing is performed by `WiFiWorker`, a dedicated ESPressio `PrecisionThread`. WiFi does not create a private FreeRTOS task or duplicate scheduling/rate-limiting logic.

Persistence remains developer-selected and backend-neutral. WiFi does not depend on LittleFS, NVS, SD, or any other concrete persistence mechanism.

Security remains optional and enters only when the developer selects protected configuration persistence. WiFi never chooses an algorithm or owns keys.

Event and Command integrations are owned by WiFi to preserve dependency direction. Event and Command themselves do not learn about WiFi.

Serial is terminal/downstream: `Serial - - -> WiFi`; `WiFi -> Serial` never exists.

Web/HTTP/WebSocket/browser infrastructure is intentionally outside ESPressio WiFi and belongs in ESPressio Web.
