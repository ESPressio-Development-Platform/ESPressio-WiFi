#pragma once

#include <ESPressio_IObserver.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

class IWiFiObserver : public virtual Observable::IObserver {
public:
    virtual ~IWiFiObserver() = default;
    virtual void OnWiFiModeChanged(WiFiMode, WiFiMode) {}
    virtual void OnClientStateChanged(const ClientRuntimeState&, const ClientRuntimeState&) {}
    virtual void OnAccessPointStateChanged(const AccessPointRuntimeState&, const AccessPointRuntimeState&) {}
    virtual void OnScanStateChanged(ScanState, ScanState) {}
    virtual void OnScanCompleted(const std::vector<ScanResult>&) {}
    virtual void OnAccessPointStationConnected(const MacAddress&) {}
    virtual void OnAccessPointStationDisconnected(const MacAddress&) {}
    virtual void OnClientIPAddressAcquired(const NetworkAddress&) {}
    virtual void OnClientIPAddressLost() {}
    virtual void OnClientNetworkSelectionChanged(
        const ClientNetworkSelectionRuntimeState&,
        const ClientNetworkSelectionRuntimeState&
    ) {}
    virtual void OnClientNetworkSelected(const ClientNetworkCandidate&) {}
    virtual void OnClientNoKnownNetworkAvailable() {}
};

} // namespace ESPressio::WiFi
