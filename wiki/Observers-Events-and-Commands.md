# Observers, Events and Commands

WiFi exposes synchronous lifecycle observation directly and optional Event/Command adapters for higher-level integration.

## `IWiFiObserver`

Use the native WiFi Observer surface when a component needs immediate lifecycle notification for application-level state changes such as connection, selection, scan or AP-until-Client transitions.

Callbacks are invoked after internal WiFi state locks are released.

## `IWiFiRadioObserver`

Use the dedicated radio observer for low-level shared-radio coordination. This is the appropriate surface for components such as ESP-Now that need authoritative mode/channel/scan/interface transitions synchronously.

## Event bridge

The optional WiFi Event bridge observes WiFi and emits asynchronous Events for subscribers that should not execute inside the synchronous lifecycle callback.

WiFi remains authoritative; Event is an adapter, not a second state machine.

## Command handler

The optional Command integration exposes operator/machine operations such as status, mode selection, scanning, remembered-network management, AP-until-Client controls and configuration save/load.

Commands reuse WiFiManager's authoritative API and must not expose plaintext credentials.

## Dependency discipline

Observable, Serializable and Threads are part of the core architecture. Event and Command remain optional integrations and should not be included by the normal core umbrella unless selected.

## Choosing the path

Use direct/Observable callbacks for immediate lifecycle coupling, Event for asynchronous fan-out, and Command for requested operations. Do not use Event to perform low-level radio coordination that must be synchronized with the actual transition.