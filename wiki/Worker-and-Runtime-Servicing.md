# Worker and Runtime Servicing

`WiFiWorker` provides autonomous runtime servicing through ESPressio Threads `PrecisionThread`.

Normal applications should not call `Poll()` from the Arduino loop.

## Runtime configuration

Worker scheduling is intentionally separate from persisted WiFi configuration:

```cpp
WiFiWorkerConfiguration runtime;
runtime.IterationPeriodMilliseconds = 25;
runtime.DesiredExecutionBudgetMilliseconds = 5;

WiFiWorker worker(wifi, runtime);
```

The default service period is 50 ms and the default desired execution budget is 5 ms.

## Bumping the worker

Explicit operations such as configuration changes, scans and connection requests can wake/bump the worker so they do not wait unnecessarily for the next normal period.

## Ownership

Threads owns worker lifecycle, precision scheduling, skipped-iteration accounting and execution diagnostics. WiFi owns what work must be serviced during an iteration.

## Deterministic/manual servicing

`WiFiManager::ProcessOnce()` remains useful for deterministic tests and specialist integrations. Historical `Poll()` is a compatibility alias; neither is the normal application servicing model.

## Extension rule

Do not create independent native WiFi maintenance tasks for new WiFi features when they can be integrated into the existing worker/state machine. Multiple unrelated service contexts make state transitions, locking and radio coordination harder to reason about.