#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IWiFiObserver.hpp"
#include "ESPressio_IWiFiConfigurationStore.hpp"

namespace ESPressio::WiFi {

enum class WiFiStatus : uint8_t { Success, InvalidConfiguration, NotSupported, Busy, PlatformError };

enum class WiFiPlatformEventKind : uint8_t {
    AccessPointStationConnected,
    AccessPointStationDisconnected,
    ClientIPAddressAcquired,
    ClientIPAddressLost
};

struct WiFiPlatformEvent {
    WiFiPlatformEventKind Kind{};
    MacAddress Station{};
    NetworkAddress Network{};
};

class IWiFiPlatform {
public:
    virtual ~IWiFiPlatform() = default;
    virtual WiFiStatus Apply(const WiFiConfiguration&) = 0;
    virtual WiFiStatus Disable() = 0;
    virtual WiFiStatus ConnectClient() = 0;
    virtual WiFiStatus DisconnectClient() = 0;
    virtual WiFiStatus StartAccessPoint() = 0;
    virtual WiFiStatus StopAccessPoint() = 0;
    virtual WiFiStatus StartScan() = 0;
    virtual WiFiStatus Poll(WiFiRuntimeState&, std::vector<ScanResult>*, std::vector<WiFiPlatformEvent>*) = 0;
};

class WiFiManager {
private:
    class ManagerObservable final : public Observable::ThreadSafeObservable {
        template<typename F> void Notify(F&& callback) {
            ExecuteNotification([&](NotificationContext& n) {
                n.WithObservers<IWiFiObserver>([&](IWiFiObserver* o) {
                    try { callback(o); } catch (...) {}
                });
            });
        }
    public:
        void Mode(WiFiMode before, WiFiMode after) { Notify([&](IWiFiObserver* o){ o->OnWiFiModeChanged(before, after); }); }
        void Client(const ClientRuntimeState& before, const ClientRuntimeState& after) { Notify([&](IWiFiObserver* o){ o->OnClientStateChanged(before, after); }); }
        void AccessPoint(const AccessPointRuntimeState& before, const AccessPointRuntimeState& after) { Notify([&](IWiFiObserver* o){ o->OnAccessPointStateChanged(before, after); }); }
        void ScanState(ScanState before, ScanState after) { Notify([&](IWiFiObserver* o){ o->OnScanStateChanged(before, after); }); }
        void ScanComplete(const std::vector<ScanResult>& results) { Notify([&](IWiFiObserver* o){ o->OnScanCompleted(results); }); }
        void APStationConnected(const MacAddress& mac) { Notify([&](IWiFiObserver* o){ o->OnAccessPointStationConnected(mac); }); }
        void APStationDisconnected(const MacAddress& mac) { Notify([&](IWiFiObserver* o){ o->OnAccessPointStationDisconnected(mac); }); }
        void IP(const NetworkAddress& network) { Notify([&](IWiFiObserver* o){ o->OnClientIPAddressAcquired(network); }); }
        void IPLost() { Notify([](IWiFiObserver* o){ o->OnClientIPAddressLost(); }); }
    };

public:
    using ModeCallback = std::function<void(WiFiMode, WiFiMode)>;
    using ClientCallback = std::function<void(const ClientRuntimeState&, const ClientRuntimeState&)>;
    using AccessPointCallback = std::function<void(const AccessPointRuntimeState&, const AccessPointRuntimeState&)>;
    using ScanStateCallback = std::function<void(ScanState, ScanState)>;
    using ScanCallback = std::function<void(const std::vector<ScanResult>&)>;
    using StationCallback = std::function<void(const MacAddress&)>;
    using IPAddressCallback = std::function<void(const NetworkAddress&)>;
    using IPLostCallback = std::function<void()>;

    explicit WiFiManager(IWiFiPlatform& platform) : _platform(platform) {}

    const WiFiConfiguration& Configuration() const noexcept { return _configuration; }
    WiFiConfiguration& MutableConfiguration() noexcept { return _configuration; }
    const WiFiRuntimeState& State() const noexcept { return _state; }
    const std::vector<ScanResult>& LastScanResults() const noexcept { return _lastScanResults; }

    void SetConfigurationStore(IWiFiConfigurationStore* store) noexcept { _configurationStore = store; }
    IWiFiConfigurationStore* ConfigurationStore() const noexcept { return _configurationStore; }

    WiFiConfigurationStoreResult SaveConfiguration() {
        if (_configurationStore == nullptr) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::NotConfigured,
                "No WiFi configuration store is configured"
            );
        }
        return _configurationStore->Save(_configuration);
    }

    WiFiConfigurationStoreResult LoadConfiguration(bool apply = true) {
        if (_configurationStore == nullptr) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::NotConfigured,
                "No WiFi configuration store is configured"
            );
        }
        WiFiConfiguration loaded;
        auto result = _configurationStore->Load(loaded);
        if (!result) return result;
        _configuration = std::move(loaded);
        if (apply && _platform.Apply(_configuration) != WiFiStatus::Success) {
            return WiFiConfigurationStoreResult::Fail(
                WiFiConfigurationStoreStatus::StorageError,
                "Configuration loaded but could not be applied to the WiFi platform"
            );
        }
        return result;
    }

    Observable::ObserverHandlePtr RegisterObserver(IWiFiObserver* observer) { return _observable->RegisterObserver(observer); }
    void UnregisterObserver(IWiFiObserver* observer) { _observable->UnregisterObserver(observer); }

    WiFiStatus Configure(WiFiConfiguration configuration) {
        _configuration = std::move(configuration);
        return _platform.Apply(_configuration);
    }

    WiFiStatus ApplyConfiguration() { return _platform.Apply(_configuration); }
    WiFiStatus Disable() { return _platform.Disable(); }
    WiFiStatus ConnectClient() { return _platform.ConnectClient(); }
    WiFiStatus DisconnectClient() { return _platform.DisconnectClient(); }
    WiFiStatus StartAccessPoint() { return _platform.StartAccessPoint(); }
    WiFiStatus StopAccessPoint() { return _platform.StopAccessPoint(); }
    WiFiStatus Scan() { return _platform.StartScan(); }

    void OnModeChanged(ModeCallback cb) { _modeCallback = std::move(cb); }
    void OnClientStateChanged(ClientCallback cb) { _clientCallback = std::move(cb); }
    void OnAccessPointStateChanged(AccessPointCallback cb) { _apCallback = std::move(cb); }
    void OnScanStateChanged(ScanStateCallback cb) { _scanStateCallback = std::move(cb); }
    void OnScanCompleted(ScanCallback cb) { _scanCallback = std::move(cb); }
    void OnAccessPointStationConnected(StationCallback cb) { _stationConnected = std::move(cb); }
    void OnAccessPointStationDisconnected(StationCallback cb) { _stationDisconnected = std::move(cb); }
    void OnClientIPAddressAcquired(IPAddressCallback cb) { _ipAcquired = std::move(cb); }
    void OnClientIPAddressLost(IPLostCallback cb) { _ipLost = std::move(cb); }

    WiFiStatus Poll() {
        WiFiRuntimeState next = _state;
        std::vector<ScanResult> scan;
        std::vector<WiFiPlatformEvent> events;
        const auto status = _platform.Poll(next, &scan, &events);
        if (status != WiFiStatus::Success) return status;

        if (next.Revision != _state.Revision) {
            const auto before = _state;
            _state = next;

            if (before.Mode != next.Mode) {
                if (_modeCallback) _modeCallback(before.Mode, next.Mode);
                _observable->Mode(before.Mode, next.Mode);
            }

            if (before.Client.State != next.Client.State || before.Client.Network.Address != next.Client.Network.Address) {
                if (_clientCallback) _clientCallback(before.Client, next.Client);
                _observable->Client(before.Client, next.Client);
            }

            if (before.AccessPoint.State != next.AccessPoint.State ||
                before.AccessPoint.ConnectedStations != next.AccessPoint.ConnectedStations) {
                if (_apCallback) _apCallback(before.AccessPoint, next.AccessPoint);
                _observable->AccessPoint(before.AccessPoint, next.AccessPoint);
            }

            if (before.Scan != next.Scan) {
                if (_scanStateCallback) _scanStateCallback(before.Scan, next.Scan);
                _observable->ScanState(before.Scan, next.Scan);
            }
        }

        if (!scan.empty()) {
            _lastScanResults = scan;
            if (_scanCallback) _scanCallback(_lastScanResults);
            _observable->ScanComplete(_lastScanResults);
        }

        for (const auto& event : events) {
            switch (event.Kind) {
                case WiFiPlatformEventKind::AccessPointStationConnected:
                    if (_stationConnected) _stationConnected(event.Station);
                    _observable->APStationConnected(event.Station);
                    break;
                case WiFiPlatformEventKind::AccessPointStationDisconnected:
                    if (_stationDisconnected) _stationDisconnected(event.Station);
                    _observable->APStationDisconnected(event.Station);
                    break;
                case WiFiPlatformEventKind::ClientIPAddressAcquired:
                    if (_ipAcquired) _ipAcquired(event.Network);
                    _observable->IP(event.Network);
                    break;
                case WiFiPlatformEventKind::ClientIPAddressLost:
                    if (_ipLost) _ipLost();
                    _observable->IPLost();
                    break;
            }
        }
        return WiFiStatus::Success;
    }

private:
    IWiFiPlatform& _platform;
    IWiFiConfigurationStore* _configurationStore = nullptr;
    WiFiConfiguration _configuration{};
    WiFiRuntimeState _state{};
    std::vector<ScanResult> _lastScanResults;
    std::shared_ptr<ManagerObservable> _observable = std::make_shared<ManagerObservable>();
    ModeCallback _modeCallback;
    ClientCallback _clientCallback;
    AccessPointCallback _apCallback;
    ScanStateCallback _scanStateCallback;
    ScanCallback _scanCallback;
    StationCallback _stationConnected;
    StationCallback _stationDisconnected;
    IPAddressCallback _ipAcquired;
    IPLostCallback _ipLost;
};

} // namespace ESPressio::WiFi
