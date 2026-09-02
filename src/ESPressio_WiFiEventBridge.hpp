#pragma once

#include "ESPressio_IWiFiObserver.hpp"
#include "ESPressio_WiFi.hpp"
#include "ESPressio_WiFiEvents.hpp"

namespace ESPressio::Event {

/// <summary>Bridges Wi-Fi observer callbacks into queued ESPressio Event instances.</summary>
class WiFiEventBridge final : public WiFi::IWiFiObserver {
public:
    /// <summary>Registers the bridge with a WiFiManager.</summary>
    /// <returns><c>true</c> when the bridge is registered or was already initialized.</returns>
    bool Initialize(WiFi::WiFiManager& manager) {
        if (_initialized) return true;
        _observer = manager.RegisterObserver(this);
        _initialized = static_cast<bool>(_observer);
        return _initialized;
    }

    /// <summary>Unregisters the bridge and releases its observer handle.</summary>
    void Shutdown() { _observer.reset(); _initialized = false; }
    /// <summary>Indicates whether the bridge currently holds an active Wi-Fi observer registration.</summary>
    bool IsInitialized() const noexcept { return _initialized; }

    /// <inheritdoc/>
    void OnWiFiModeChanged(WiFi::WiFiMode before, WiFi::WiFiMode after) override {
        (new WiFiModeChangedEvent(before, after))->Queue();
    }
    /// <inheritdoc/>
    void OnClientStateChanged(const WiFi::ClientRuntimeState& before, const WiFi::ClientRuntimeState& after) override {
        (new WiFiClientStateChangedEvent(before, after))->Queue();
    }
    /// <inheritdoc/>
    void OnAccessPointStateChanged(const WiFi::AccessPointRuntimeState& before, const WiFi::AccessPointRuntimeState& after) override {
        (new WiFiAccessPointStateChangedEvent(before, after))->Queue();
    }
    /// <inheritdoc/>
    void OnAPUntilClientStateChanged(
        const WiFi::APUntilClientRuntimeState& before,
        const WiFi::APUntilClientRuntimeState& after
    ) override {
        (new WiFiAPUntilClientStateChangedEvent(before, after))->Queue();
    }
    /// <inheritdoc/>
    void OnScanStateChanged(WiFi::ScanState before, WiFi::ScanState after) override {
        (new WiFiScanStateChangedEvent(before, after))->Queue();
    }
    /// <inheritdoc/>
    void OnScanCompleted(const WiFi::WiFiVector<WiFi::ScanResult>& results) override {
        (new WiFiScanCompletedEvent(results))->Queue();
    }
    /// <inheritdoc/>
    void OnAccessPointStationConnected(const WiFi::MacAddress& station) override {
        (new WiFiAccessPointStationConnectedEvent(station))->Queue();
    }
    /// <inheritdoc/>
    void OnAccessPointStationDisconnected(const WiFi::MacAddress& station) override {
        (new WiFiAccessPointStationDisconnectedEvent(station))->Queue();
    }
    /// <inheritdoc/>
    void OnClientIPAddressAcquired(const WiFi::NetworkAddress& network) override {
        (new WiFiClientIPAddressAcquiredEvent(network))->Queue();
    }
    /// <inheritdoc/>
    void OnClientIPAddressLost() override { (new WiFiClientIPAddressLostEvent())->Queue(); }
    /// <inheritdoc/>
    void OnClientNetworkSelectionChanged(
        const WiFi::ClientNetworkSelectionRuntimeState& before,
        const WiFi::ClientNetworkSelectionRuntimeState& after
    ) override {
        (new WiFiClientNetworkSelectionChangedEvent(before, after))->Queue();
    }
    /// <inheritdoc/>
    void OnClientNetworkSelected(const WiFi::ClientNetworkCandidate& selected) override {
        (new WiFiClientNetworkSelectedEvent(selected))->Queue();
    }
    /// <inheritdoc/>
    void OnClientNoKnownNetworkAvailable() override {
        (new WiFiClientNoKnownNetworkAvailableEvent())->Queue();
    }

private:
    Observable::ObserverHandlePtr _observer;
    bool _initialized = false;
};

} // namespace ESPressio::Event
