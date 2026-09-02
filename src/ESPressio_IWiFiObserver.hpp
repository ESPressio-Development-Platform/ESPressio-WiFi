#pragma once

#include <ESPressio_IObserver.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

/// <summary>Observer contract for Wi-Fi mode, client, access-point, scan, and network-selection state changes.</summary>
class IWiFiObserver : public virtual Observable::IObserver {
public:
    virtual ~IWiFiObserver() = default;
    /// <summary>Called when the configured Wi-Fi operating mode changes.</summary>
    virtual void OnWiFiModeChanged(WiFiMode, WiFiMode) {}
    /// <summary>Called when client runtime state changes.</summary>
    virtual void OnClientStateChanged(const ClientRuntimeState&, const ClientRuntimeState&) {}
    /// <summary>Called when access-point runtime state changes.</summary>
    virtual void OnAccessPointStateChanged(const AccessPointRuntimeState&, const AccessPointRuntimeState&) {}
    /// <summary>Called when AP-until-client runtime state changes.</summary>
    virtual void OnAPUntilClientStateChanged(const APUntilClientRuntimeState&, const APUntilClientRuntimeState&) {}
    /// <summary>Called when Wi-Fi scan state changes.</summary>
    virtual void OnScanStateChanged(ScanState, ScanState) {}
    /// <summary>Called with the completed Wi-Fi scan result set retained in externally preferred storage.</summary>
    virtual void OnScanCompleted(const WiFiVector<ScanResult>&) {}
    /// <summary>Called when a station associates with the local access point.</summary>
    virtual void OnAccessPointStationConnected(const MacAddress&) {}
    /// <summary>Called when a station disconnects from the local access point.</summary>
    virtual void OnAccessPointStationDisconnected(const MacAddress&) {}
    /// <summary>Called when the client interface acquires an IP address.</summary>
    virtual void OnClientIPAddressAcquired(const NetworkAddress&) {}
    /// <summary>Called when the client interface loses its IP address.</summary>
    virtual void OnClientIPAddressLost() {}
    /// <summary>Called when preferred-client-network selection state changes.</summary>
    virtual void OnClientNetworkSelectionChanged(
        const ClientNetworkSelectionRuntimeState&,
        const ClientNetworkSelectionRuntimeState&
    ) {}
    /// <summary>Called when a known client-network candidate is selected.</summary>
    virtual void OnClientNetworkSelected(const ClientNetworkCandidate&) {}
    /// <summary>Called when scanning finds no known client network that can be selected.</summary>
    virtual void OnClientNoKnownNetworkAvailable() {}
};

} // namespace ESPressio::WiFi
