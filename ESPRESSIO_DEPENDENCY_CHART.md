# ESPressio WiFi dependency position — Current Released Generation

Arrows point from the consuming library to the library it consumes. Solid edges are required; dashed edges are opt-in integrations.

```text
WiFi
    -> Observable main
    -> Serializable main
    -> Threads main

WiFi persistence integration
    - - -> Persistence main

WiFi protected persistence integration
    - - -> Persistence main
    - - -> Serializable main
    - - -> Security main

WiFi Event bridge
    - - -> Event main

WiFi Command handler
    - - -> Command main

Serial
    - - -> WiFi main
```

The coordinated released generation validated by WiFi is:

```text
Observable
Serializable
Units
Timing
Threads
Event
Command
Security
Persistence
Sockets
ESP-Now
WiFi
Serial
```

Observable is part of the normal WiFi contract because externally meaningful WiFi lifecycle and remembered-network selection transitions are observable.

Serializable is foundational because WiFi configuration—including all remembered Client profiles, credentials, priorities and addressing—is Serializable.

Threads is required because normal WiFi runtime servicing is performed by `WiFiWorker`, a dedicated ESPressio `PrecisionThread`. WiFi does not create a private FreeRTOS task or duplicate scheduling/rate-limiting logic.

Persistence remains developer-selected and backend-neutral. Security remains optional and enters only when the developer selects protected configuration persistence. Event and Command integrations are owned by WiFi to preserve dependency direction.

Serial is terminal/downstream: `Serial - - -> WiFi`; `WiFi -> Serial` never exists. Web/HTTP/WebSocket/browser infrastructure remains outside ESPressio WiFi and belongs in ESPressio Web.
