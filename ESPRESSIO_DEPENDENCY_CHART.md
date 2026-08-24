# ESPressio WiFi 0.2.0 dependency position — Current Released Generation

Arrows point from the consuming library to the library it consumes. Solid edges are required; dashed edges are opt-in integrations.

```text
WiFi 0.2.0
    -> Observable >= 3.0.2 < 4.0.0
    -> Serializable >= 0.11.3 < 1.0.0
    -> Threads >= 3.1.7 < 4.0.0

WiFi persistence integration
    - - -> Persistence >= 0.3.2 < 1.0.0

WiFi protected persistence integration
    - - -> Persistence >= 0.3.2 < 1.0.0
    - - -> Serializable >= 0.11.3 < 1.0.0
    - - -> Security >= 0.4.2 < 1.0.0

WiFi Event bridge
    - - -> Event >= 6.0.3 < 7.0.0

WiFi Command handler
    - - -> Command >= 1.0.3 < 2.0.0

Serial 0.8.1
    - - -> WiFi >= 0.2.0 < 1.0.0
```

The coordinated released generation validated by WiFi 0.2.0 is:

```text
Observable    3.0.2
Serializable  0.11.3
Units         0.2.7
Timing        2.2.8
Threads       3.1.7
Event         6.0.3
Command       1.0.3
Security      0.4.2
Persistence   0.3.2
Sockets       0.7.3
ESP-Now       0.8.3
WiFi          0.2.0
Serial        0.8.1
```

Observable is part of the normal WiFi contract because externally meaningful WiFi lifecycle and remembered-network selection transitions are observable.

Serializable is foundational because WiFi configuration—including all remembered Client profiles, credentials, priorities and addressing—is Serializable.

Threads is required because normal WiFi runtime servicing is performed by `WiFiWorker`, a dedicated ESPressio `PrecisionThread`. WiFi does not create a private FreeRTOS task or duplicate scheduling/rate-limiting logic.

Persistence remains developer-selected and backend-neutral. Security remains optional and enters only when the developer selects protected configuration persistence. Event and Command integrations are owned by WiFi to preserve dependency direction.

Serial is terminal/downstream: `Serial - - -> WiFi`; `WiFi -> Serial` never exists. Web/HTTP/WebSocket/browser infrastructure remains outside ESPressio WiFi and belongs in ESPressio Web.
