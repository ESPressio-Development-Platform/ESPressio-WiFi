#pragma once

#include <ESPressio_IObserver.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

/// <summary>Low-level observer contract for infrastructure that shares the ESP32 Wi-Fi radio, such as ESP-NOW.</summary>
/// <remarks>This is intentionally separate from IWiFiObserver, whose callbacks describe application-level Wi-Fi behavior rather than radio coexistence.</remarks>
class IWiFiRadioObserver : public virtual Observable::IObserver {
public:
    virtual ~IWiFiRadioObserver() = default;

    /// <summary>Called immediately before a radio transition begins.</summary>
    virtual void OnWiFiRadioTransitionBeginning(
        const WiFiRadioState& before,
        WiFiRadioTransitionReason reason
    ) {}

    /// <summary>Called after a radio transition completes.</summary>
    virtual void OnWiFiRadioTransitionCompleted(
        const WiFiRadioState& before,
        const WiFiRadioState& after,
        WiFiRadioTransitionReason reason
    ) {}

    /// <summary>Called whenever the effective shared-radio state changes.</summary>
    virtual void OnWiFiRadioStateChanged(
        const WiFiRadioState& before,
        const WiFiRadioState& after
    ) {}

    /// <summary>Called before a Wi-Fi scan begins using the shared radio.</summary>
    virtual void OnWiFiRadioScanBeginning(const WiFiRadioState& before) {}
    /// <summary>Called after a Wi-Fi scan releases or reconfigures the shared radio.</summary>
    virtual void OnWiFiRadioScanCompleted(const WiFiRadioState& after) {}
};

} // namespace ESPressio::WiFi
