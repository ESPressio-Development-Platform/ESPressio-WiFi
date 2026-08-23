#pragma once

#include "ESPressio_IWiFiObserver.hpp"
#include "ESPressio_WiFi.hpp"
#include "ESPressio_WiFiEvents.hpp"

namespace ESPressio::Event {

class WiFiEventBridge final : public WiFi::IWiFiObserver {
public:
    bool Initialize(WiFi::WiFiManager& manager) {
        if (_initialized) return true;
        _observer = manager.RegisterObserver(this);
        _initialized = static_cast<bool>(_observer);
        return _initialized;
    }

    void Shutdown() { _observer.reset(); _initialized = false; }
    bool IsInitialized() const noexcept { return _initialized; }

    void OnWiFiModeChanged(WiFi::WiFiMode before, WiFi::WiFiMode after) override {
        (new WiFiModeChangedEvent(before, after))->Queue();
    }
    void OnClientStateChanged(const WiFi::ClientRuntimeState& before, const WiFi::ClientRuntimeState& after) override {
        (new WiFiClientStateChangedEvent(before, after))->Queue();
    }
    void OnAccessPointStateChanged(const WiFi::AccessPointRuntimeState& before, const WiFi::AccessPointRuntimeState& after) override {
        (new WiFiAccessPointStateChangedEvent(before, after))->Queue();
    }
    void OnAPUntilClientStateChanged(
        const WiFi::APUntilClientRuntimeState& before,
        const WiFi::APUntilClientRuntimeState& after
    ) override {
        (new WiFiAPUntilClientStateChangedEvent(before, after))->Queue();
    }
    void OnScanStateChanged(WiFi::ScanState before, WiFi::ScanState after) override {
        (new WiFiScanStateChangedEvent(before, after))->Queue();
    }
    void OnScanCompleted(const std::vector<WiFi::ScanResult>& results) override {
        (new WiFiScanCompletedEvent(results))->Queue();
    }
    void OnAccessPointStationConnected(const WiFi::MacAddress& station) override {
        (new WiFiAccessPointStationConnectedEvent(station))->Queue();
    }
    void OnAccessPointStationDisconnected(const WiFi::MacAddress& station) override {
        (new WiFiAccessPointStationDisconnectedEvent(station))->Queue();
    }
    void OnClientIPAddressAcquired(const WiFi::NetworkAddress& network) override {
        (new WiFiClientIPAddressAcquiredEvent(network))->Queue();
    }
    void OnClientIPAddressLost() override { (new WiFiClientIPAddressLostEvent())->Queue(); }
    void OnClientNetworkSelectionChanged(
        const WiFi::ClientNetworkSelectionRuntimeState& before,
        const WiFi::ClientNetworkSelectionRuntimeState& after
    ) override {
        (new WiFiClientNetworkSelectionChangedEvent(before, after))->Queue();
    }
    void OnClientNetworkSelected(const WiFi::ClientNetworkCandidate& selected) override {
        (new WiFiClientNetworkSelectedEvent(selected))->Queue();
    }
    void OnClientNoKnownNetworkAvailable() override {
        (new WiFiClientNoKnownNetworkAvailableEvent())->Queue();
    }

private:
    Observable::ObserverHandlePtr _observer;
    bool _initialized = false;
};

} // namespace ESPressio::Event
