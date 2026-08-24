#pragma once

#include <ESPressio_IObserver.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

// Low-level infrastructure observer for consumers that share the ESP32 WiFi
// radio (for example ESP-NOW). This is deliberately separate from
// IWiFiObserver, whose callbacks describe application-level WiFi semantics.
class IWiFiRadioObserver : public virtual Observable::IObserver {
public:
    virtual ~IWiFiRadioObserver() = default;

    virtual void OnWiFiRadioTransitionBeginning(
        const WiFiRadioState& before,
        WiFiRadioTransitionReason reason
    ) {}

    virtual void OnWiFiRadioTransitionCompleted(
        const WiFiRadioState& before,
        const WiFiRadioState& after,
        WiFiRadioTransitionReason reason
    ) {}

    virtual void OnWiFiRadioStateChanged(
        const WiFiRadioState& before,
        const WiFiRadioState& after
    ) {}

    virtual void OnWiFiRadioScanBeginning(const WiFiRadioState& before) {}
    virtual void OnWiFiRadioScanCompleted(const WiFiRadioState& after) {}
};

} // namespace ESPressio::WiFi
