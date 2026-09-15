#pragma once

#include <atomic>
#include <cstdint>
#include <utility>

#include <ESPressio_Event.hpp>
#include "ESPressio_IWiFiObserver.hpp"
#include "ESPressio_WiFi.hpp"
#include "ESPressio_WiFiEvents.hpp"

namespace ESPressio::Event {

/// Bridges caller-owned WiFiManager observer notifications into the final typed
/// Event runtime. The bridge owns only its WiFi observer registration and a
/// bounded diagnostic drop counter: Event owns occurrence allocation, admission,
/// queueing and dispatch lifecycle.
class WiFiEventBridge final : public WiFi::IWiFiObserver {
public:
    bool Initialize(WiFi::WiFiManager& manager) {
        if (_observer) return true;
        _observer = manager.RegisterObserver(this);
        return static_cast<bool>(_observer);
    }

    void Shutdown() noexcept { _observer.reset(); }
    bool IsInitialized() const noexcept { return static_cast<bool>(_observer); }
    std::uint64_t UnavailableDispatches() const noexcept {
        return _unavailableDispatches.load(std::memory_order_relaxed);
    }

    void OnWiFiModeChanged(WiFi::WiFiMode before, WiFi::WiFiMode after) override {
        Emit<WiFiModeChangedEvent>(before, after);
    }
    void OnClientStateChanged(const WiFi::ClientRuntimeState& before, const WiFi::ClientRuntimeState& after) override {
        Emit<WiFiClientStateChangedEvent>(before, after);
    }
    void OnAccessPointStateChanged(const WiFi::AccessPointRuntimeState& before, const WiFi::AccessPointRuntimeState& after) override {
        Emit<WiFiAccessPointStateChangedEvent>(before, after);
    }
    void OnAPUntilClientStateChanged(
        const WiFi::APUntilClientRuntimeState& before,
        const WiFi::APUntilClientRuntimeState& after
    ) override {
        Emit<WiFiAPUntilClientStateChangedEvent>(before, after);
    }
    void OnScanStateChanged(WiFi::ScanState before, WiFi::ScanState after) override {
        Emit<WiFiScanStateChangedEvent>(before, after);
    }
    void OnScanCompleted(const WiFi::WiFiVector<WiFi::ScanResult>& results) override {
        Emit<WiFiScanCompletedEvent>(results);
    }
    void OnAccessPointStationConnected(const WiFi::MacAddress& station) override {
        Emit<WiFiAccessPointStationConnectedEvent>(station);
    }
    void OnAccessPointStationDisconnected(const WiFi::MacAddress& station) override {
        Emit<WiFiAccessPointStationDisconnectedEvent>(station);
    }
    void OnClientIPAddressAcquired(const WiFi::NetworkAddress& network) override {
        Emit<WiFiClientIPAddressAcquiredEvent>(network);
    }
    void OnClientIPAddressLost() override { Emit<WiFiClientIPAddressLostEvent>(); }
    void OnClientNetworkSelectionChanged(
        const WiFi::ClientNetworkSelectionRuntimeState& before,
        const WiFi::ClientNetworkSelectionRuntimeState& after
    ) override {
        Emit<WiFiClientNetworkSelectionChangedEvent>(before, after);
    }
    void OnClientNetworkSelected(const WiFi::ClientNetworkCandidate& selected) override {
        Emit<WiFiClientNetworkSelectedEvent>(selected);
    }
    void OnClientNoKnownNetworkAvailable() override {
        Emit<WiFiClientNoKnownNetworkAvailableEvent>();
    }

private:
    template<class TEvent, class... Args>
    void Emit(Args&&... args) noexcept {
        try {
            if (!TEvent::TryDispatch(std::forward<Args>(args)...)) {
                _unavailableDispatches.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (...) {
            _unavailableDispatches.fetch_add(1, std::memory_order_relaxed);
        }
    }

    Observable::ObserverHandlePtr _observer;
    std::atomic<std::uint64_t> _unavailableDispatches{0};
};

} // namespace ESPressio::Event
