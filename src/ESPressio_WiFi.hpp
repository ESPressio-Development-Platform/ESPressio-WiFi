#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
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
    virtual WiFiStatus ConnectClient(const ClientNetworkProfile&) { return WiFiStatus::NotSupported; }
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
        void Selection(const ClientNetworkSelectionRuntimeState& before, const ClientNetworkSelectionRuntimeState& after) { Notify([&](IWiFiObserver* o){ o->OnClientNetworkSelectionChanged(before, after); }); }
        void Selected(const ClientNetworkCandidate& selected) { Notify([&](IWiFiObserver* o){ o->OnClientNetworkSelected(selected); }); }
        void NoKnownNetwork() { Notify([](IWiFiObserver* o){ o->OnClientNoKnownNetworkAvailable(); }); }
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
    using SelectionCallback = std::function<void(const ClientNetworkSelectionRuntimeState&, const ClientNetworkSelectionRuntimeState&)>;
    using SelectedNetworkCallback = std::function<void(const ClientNetworkCandidate&)>;
    using SimpleCallback = std::function<void()>;
    using WorkSignal = std::function<void()>;

    explicit WiFiManager(IWiFiPlatform& platform) : _platform(platform) {}

    WiFiConfiguration Configuration() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _configuration;
    }

    WiFiRuntimeState State() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _state;
    }

    std::vector<ScanResult> LastScanResults() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _lastScanResults;
    }

    std::vector<ClientNetworkCandidate> EligibleClientNetworks() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _eligibleCandidates;
    }

    void SetWorkSignal(WorkSignal signal) {
        std::lock_guard<std::mutex> lock(_mutex);
        _workSignal = std::move(signal);
    }

    void SetConfigurationStore(IWiFiConfigurationStore* store) noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        _configurationStore = store;
    }

    IWiFiConfigurationStore* ConfigurationStore() const noexcept {
        std::lock_guard<std::mutex> lock(_mutex);
        return _configurationStore;
    }

    WiFiConfigurationStoreResult SaveConfiguration() {
        IWiFiConfigurationStore* store = nullptr;
        WiFiConfiguration snapshot;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            store = _configurationStore;
            snapshot = _configuration;
        }
        if (store == nullptr) {
            return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::NotConfigured, "No WiFi configuration store is configured");
        }
        return store->Save(snapshot);
    }

    WiFiConfigurationStoreResult LoadConfiguration(bool apply = true) {
        IWiFiConfigurationStore* store = nullptr;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            store = _configurationStore;
        }
        if (store == nullptr) {
            return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::NotConfigured, "No WiFi configuration store is configured");
        }
        WiFiConfiguration loaded;
        auto result = store->Load(loaded);
        if (!result) return result;
        if (apply) {
            const auto status = Configure(std::move(loaded));
            if (status != WiFiStatus::Success) {
                return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::StorageError, "Configuration loaded but could not be applied to the WiFi platform");
            }
        } else {
            std::lock_guard<std::mutex> lock(_mutex);
            _configuration = std::move(loaded);
        }
        SignalWork();
        return result;
    }

    Observable::ObserverHandlePtr RegisterObserver(IWiFiObserver* observer) { return _observable->RegisterObserver(observer); }
    void UnregisterObserver(IWiFiObserver* observer) { _observable->UnregisterObserver(observer); }

    WiFiStatus Configure(WiFiConfiguration configuration) {
        const auto status = _platform.Apply(configuration);
        if (status != WiFiStatus::Success) return status;
        bool scanOnStartup = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _configuration = std::move(configuration);
            _eligibleCandidates.clear();
            _nextCandidateIndex = 0;
            scanOnStartup = UsesClient(_configuration.Mode) && _configuration.Client.Enabled &&
                !_configuration.Client.Networks.empty() && _configuration.Client.Selection.AutomaticSelection &&
                _configuration.Client.Selection.ScanOnStartup;
        }
        if (scanOnStartup) (void)Scan();
        SignalWork();
        return status;
    }

    WiFiStatus ApplyConfiguration() { return Configure(Configuration()); }

    WiFiStatus Disable() { const auto r = _platform.Disable(); SignalWork(); return r; }
    WiFiStatus ConnectClient() { const auto r = _platform.ConnectClient(); SignalWork(); return r; }
    WiFiStatus DisconnectClient() { const auto r = _platform.DisconnectClient(); SignalWork(); return r; }
    WiFiStatus StartAccessPoint() { const auto r = _platform.StartAccessPoint(); SignalWork(); return r; }
    WiFiStatus StopAccessPoint() { const auto r = _platform.StopAccessPoint(); SignalWork(); return r; }
    WiFiStatus Scan() {
        const auto r = _platform.StartScan();
        if (r == WiFiStatus::Success) SetSelectionState(ClientNetworkSelectionState::Scanning);
        SignalWork();
        return r;
    }

    bool AddOrUpdateClientNetwork(ClientNetworkProfile profile) {
        if (profile.SSID.empty()) return false;
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = std::find_if(_configuration.Client.Networks.begin(), _configuration.Client.Networks.end(), [&](const ClientNetworkProfile& existing){ return existing.SSID == profile.SSID; });
        if (it == _configuration.Client.Networks.end()) _configuration.Client.Networks.push_back(std::move(profile));
        else *it = std::move(profile);
        return true;
    }

    bool RemoveClientNetwork(const std::string& ssid) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto& networks = _configuration.Client.Networks;
        const auto oldSize = networks.size();
        networks.erase(std::remove_if(networks.begin(), networks.end(), [&](const ClientNetworkProfile& p){ return p.SSID == ssid; }), networks.end());
        return networks.size() != oldSize;
    }

    bool SetClientNetworkPriority(const std::string& ssid, uint16_t priority) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& p : _configuration.Client.Networks) {
            if (p.SSID == ssid) { p.Priority = priority; return true; }
        }
        return false;
    }

    void OnModeChanged(ModeCallback cb) { _modeCallback = std::move(cb); }
    void OnClientStateChanged(ClientCallback cb) { _clientCallback = std::move(cb); }
    void OnAccessPointStateChanged(AccessPointCallback cb) { _apCallback = std::move(cb); }
    void OnScanStateChanged(ScanStateCallback cb) { _scanStateCallback = std::move(cb); }
    void OnScanCompleted(ScanCallback cb) { _scanCallback = std::move(cb); }
    void OnAccessPointStationConnected(StationCallback cb) { _stationConnected = std::move(cb); }
    void OnAccessPointStationDisconnected(StationCallback cb) { _stationDisconnected = std::move(cb); }
    void OnClientIPAddressAcquired(IPAddressCallback cb) { _ipAcquired = std::move(cb); }
    void OnClientIPAddressLost(IPLostCallback cb) { _ipLost = std::move(cb); }
    void OnClientNetworkSelectionChanged(SelectionCallback cb) { _selectionCallback = std::move(cb); }
    void OnClientNetworkSelected(SelectedNetworkCallback cb) { _selectedCallback = std::move(cb); }
    void OnClientNoKnownNetworkAvailable(SimpleCallback cb) { _noKnownNetworkCallback = std::move(cb); }

    WiFiStatus ProcessOnce() {
        WiFiRuntimeState next;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            next = _state;
        }

        std::vector<ScanResult> scan;
        std::vector<WiFiPlatformEvent> events;
        const auto status = _platform.Poll(next, &scan, &events);
        if (status != WiFiStatus::Success) return status;

        WiFiRuntimeState before;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            before = _state;
            _state = next;
            if (!scan.empty()) _lastScanResults = scan;
        }

        NotifyStateTransitions(before, next);

        if (!scan.empty()) {
            if (_scanCallback) _scanCallback(scan);
            _observable->ScanComplete(scan);
            HandleCompletedScan(scan);
        }

        for (const auto& event : events) NotifyPlatformEvent(event);

        HandleConnectionProgress(before.Client.State, next.Client.State);
        return WiFiStatus::Success;
    }

    // Kept as a compatibility alias for 0.1.x callers. 0.2.x applications should
    // use WiFiWorker and do not need to call this from loop().
    WiFiStatus Poll() { return ProcessOnce(); }

private:
    static bool UsesClient(WiFiMode mode) { return mode == WiFiMode::Client || mode == WiFiMode::AccessPointClient; }

    void SignalWork() {
        WorkSignal signal;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            signal = _workSignal;
        }
        if (signal) signal();
    }

    void SetSelectionState(ClientNetworkSelectionState state) {
        ClientNetworkSelectionRuntimeState before;
        ClientNetworkSelectionRuntimeState after;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            before = _state.Client.Selection;
            _state.Client.Selection.State = state;
            after = _state.Client.Selection;
        }
        if (before.State != after.State) {
            if (_selectionCallback) _selectionCallback(before, after);
            _observable->Selection(before, after);
        }
    }

    std::vector<ClientNetworkCandidate> BuildCandidates(const std::vector<ScanResult>& scan) const {
        WiFiConfiguration config;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            config = _configuration;
        }
        std::vector<ClientNetworkCandidate> candidates;
        for (std::size_t profileIndex = 0; profileIndex < config.Client.Networks.size(); ++profileIndex) {
            const auto& profile = config.Client.Networks[profileIndex];
            if (!profile.Enabled || profile.SSID.empty()) continue;
            const ScanResult* strongest = nullptr;
            for (const auto& visible : scan) {
                if (visible.SSID != profile.SSID) continue;
                if (strongest == nullptr || visible.RSSI > strongest->RSSI) strongest = &visible;
            }
            if (strongest == nullptr) continue;
            ClientNetworkCandidate c;
            c.SSID = profile.SSID;
            c.BSSID = strongest->BSSID;
            c.Priority = profile.Priority;
            c.RSSI = strongest->RSSI;
            c.Channel = strongest->Channel;
            c.ProfileIndex = profileIndex;
            candidates.push_back(std::move(c));
        }
        std::sort(candidates.begin(), candidates.end(), [](const ClientNetworkCandidate& a, const ClientNetworkCandidate& b) {
            if (a.Priority != b.Priority) return a.Priority > b.Priority;
            if (a.RSSI != b.RSSI) return a.RSSI > b.RSSI;
            return a.SSID < b.SSID;
        });
        return candidates;
    }

    void HandleCompletedScan(const std::vector<ScanResult>& scan) {
        WiFiConfiguration config;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            config = _configuration;
        }
        if (!UsesClient(config.Mode) || !config.Client.Enabled || !config.Client.Selection.AutomaticSelection || config.Client.Networks.empty()) return;

        auto candidates = BuildCandidates(scan);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _eligibleCandidates = candidates;
            _nextCandidateIndex = 0;
            _state.Client.Selection.EligibleCandidateCount = candidates.size();
        }
        if (candidates.empty()) {
            SetSelectionState(ClientNetworkSelectionState::NoKnownNetworkAvailable);
            if (_noKnownNetworkCallback) _noKnownNetworkCallback();
            _observable->NoKnownNetwork();
            return;
        }
        SetSelectionState(ClientNetworkSelectionState::Selecting);
        (void)TryNextCandidate();
    }

    WiFiStatus TryNextCandidate() {
        ClientNetworkCandidate candidate;
        ClientNetworkProfile profile;
        bool available = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_nextCandidateIndex < _eligibleCandidates.size()) {
                candidate = _eligibleCandidates[_nextCandidateIndex++];
                if (candidate.ProfileIndex < _configuration.Client.Networks.size()) {
                    profile = _configuration.Client.Networks[candidate.ProfileIndex];
                    _state.Client.Selection.SelectedSSID = candidate.SSID;
                    _state.Client.Selection.SelectedPriority = candidate.Priority;
                    _state.Client.Selection.SelectedProfileIndex = candidate.ProfileIndex;
                    available = true;
                }
            }
        }
        if (!available) {
            SetSelectionState(ClientNetworkSelectionState::Exhausted);
            return WiFiStatus::InvalidConfiguration;
        }
        if (_selectedCallback) _selectedCallback(candidate);
        _observable->Selected(candidate);
        SetSelectionState(ClientNetworkSelectionState::Connecting);
        const auto result = _platform.ConnectClient(profile);
        SignalWork();
        return result;
    }

    void HandleConnectionProgress(ClientState before, ClientState after) {
        WiFiConfiguration config;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            config = _configuration;
        }
        if (!config.Client.Selection.AutomaticSelection || config.Client.Networks.empty()) return;
        if (after == ClientState::Connected) {
            SetSelectionState(ClientNetworkSelectionState::Connected);
            return;
        }
        if (after == ClientState::Failed && before != ClientState::Failed && config.Client.Selection.TryNextOnFailure) {
            if (TryNextCandidate() == WiFiStatus::InvalidConfiguration && config.Client.Selection.ScanOnDisconnect) (void)Scan();
            return;
        }
        if (after == ClientState::Disconnected && before != ClientState::Disconnected && config.Client.Selection.ScanOnDisconnect) {
            (void)Scan();
        }
    }

    void NotifyStateTransitions(const WiFiRuntimeState& before, const WiFiRuntimeState& next) {
        if (before.Mode != next.Mode) {
            if (_modeCallback) _modeCallback(before.Mode, next.Mode);
            _observable->Mode(before.Mode, next.Mode);
        }
        if (before.Client.State != next.Client.State || before.Client.Network.Address != next.Client.Network.Address) {
            if (_clientCallback) _clientCallback(before.Client, next.Client);
            _observable->Client(before.Client, next.Client);
        }
        if (before.AccessPoint.State != next.AccessPoint.State || before.AccessPoint.ConnectedStations != next.AccessPoint.ConnectedStations) {
            if (_apCallback) _apCallback(before.AccessPoint, next.AccessPoint);
            _observable->AccessPoint(before.AccessPoint, next.AccessPoint);
        }
        if (before.Scan != next.Scan) {
            if (_scanStateCallback) _scanStateCallback(before.Scan, next.Scan);
            _observable->ScanState(before.Scan, next.Scan);
        }
    }

    void NotifyPlatformEvent(const WiFiPlatformEvent& event) {
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

    IWiFiPlatform& _platform;
    mutable std::mutex _mutex;
    IWiFiConfigurationStore* _configurationStore = nullptr;
    WiFiConfiguration _configuration{};
    WiFiRuntimeState _state{};
    std::vector<ScanResult> _lastScanResults;
    std::vector<ClientNetworkCandidate> _eligibleCandidates;
    std::size_t _nextCandidateIndex = 0;
    WorkSignal _workSignal;
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
    SelectionCallback _selectionCallback;
    SelectedNetworkCallback _selectedCallback;
    SimpleCallback _noKnownNetworkCallback;
};

} // namespace ESPressio::WiFi
