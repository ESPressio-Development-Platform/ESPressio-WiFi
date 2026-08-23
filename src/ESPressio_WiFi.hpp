#pragma once

#include <algorithm>
#include <chrono>
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
        void Provisioning(const ProvisioningRuntimeState& before, const ProvisioningRuntimeState& after) { Notify([&](IWiFiObserver* o){ o->OnProvisioningStateChanged(before, after); }); }
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
    using Clock = std::function<uint64_t()>;
    using ModeCallback = std::function<void(WiFiMode, WiFiMode)>;
    using ClientCallback = std::function<void(const ClientRuntimeState&, const ClientRuntimeState&)>;
    using AccessPointCallback = std::function<void(const AccessPointRuntimeState&, const AccessPointRuntimeState&)>;
    using ProvisioningCallback = std::function<void(const ProvisioningRuntimeState&, const ProvisioningRuntimeState&)>;
    using ScanStateCallback = std::function<void(ScanState, ScanState)>;
    using ScanCallback = std::function<void(const std::vector<ScanResult>&)>;
    using StationCallback = std::function<void(const MacAddress&)>;
    using IPAddressCallback = std::function<void(const NetworkAddress&)>;
    using IPLostCallback = std::function<void()>;
    using SelectionCallback = std::function<void(const ClientNetworkSelectionRuntimeState&, const ClientNetworkSelectionRuntimeState&)>;
    using SelectedNetworkCallback = std::function<void(const ClientNetworkCandidate&)>;
    using SimpleCallback = std::function<void()>;
    using WorkSignal = std::function<void()>;

    explicit WiFiManager(IWiFiPlatform& platform, Clock clock = {})
        : _platform(platform),
          _clock(clock ? std::move(clock) : Clock([]() -> uint64_t {
              return static_cast<uint64_t>(
                  std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch()
                  ).count()
              );
          })) {}

    WiFiConfiguration Configuration() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _configuration;
    }

    WiFiRuntimeState State() const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto state = _state;
        state.Client.Selection = _selectionState;
        state.Provisioning = _provisioningState;
        return state;
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

    void SetConfigurationStore(IWiFiConfigurationStore* store) {
        std::lock_guard<std::mutex> lock(_mutex);
        _configurationStore = store;
    }

    IWiFiConfigurationStore* ConfigurationStore() const {
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
            if (Configure(std::move(loaded)) != WiFiStatus::Success) {
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
        const auto status = WithPlatform([&]() { return _platform.Apply(configuration); });
        if (status != WiFiStatus::Success) return status;

        bool scanOnStartup = false;
        bool provisioning = false;
        bool hasRememberedNetworks = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _configuration = std::move(configuration);
            _eligibleCandidates.clear();
            _nextCandidateIndex = 0;
            _selectionState = {};
            _provisioningState = {};
            provisioning = _configuration.Mode == WiFiMode::Provisioning;
            hasRememberedNetworks = !_configuration.Client.Networks.empty();
            if (provisioning) {
                _provisioningState.State = ProvisioningState::AccessPointAvailable;
                _provisioningState.AccessPointRequired = true;
                _provisioningState.FallbackTimeoutMilliseconds =
                    _configuration.Provisioning.AccessPointFallbackTimeoutMilliseconds;
            }
            scanOnStartup = UsesClient(_configuration.Mode) && _configuration.Client.Enabled &&
                hasRememberedNetworks && _configuration.Client.Selection.AutomaticSelection &&
                _configuration.Client.Selection.ScanOnStartup;
        }

        if (scanOnStartup) {
            (void)Scan();
        } else if (provisioning && !hasRememberedNetworks) {
            SetProvisioningState(ProvisioningState::AccessPointAvailable, true, 0);
        }

        SignalWork();
        return status;
    }

    WiFiStatus ApplyConfiguration() { return Configure(Configuration()); }
    WiFiStatus Disable() { const auto r = WithPlatform([&](){ return _platform.Disable(); }); SignalWork(); return r; }
    WiFiStatus ConnectClient() { const auto r = WithPlatform([&](){ return _platform.ConnectClient(); }); SignalWork(); return r; }
    WiFiStatus DisconnectClient() { const auto r = WithPlatform([&](){ return _platform.DisconnectClient(); }); SignalWork(); return r; }
    WiFiStatus StartAccessPoint() { const auto r = WithPlatform([&](){ return _platform.StartAccessPoint(); }); SignalWork(); return r; }
    WiFiStatus StopAccessPoint() { const auto r = WithPlatform([&](){ return _platform.StopAccessPoint(); }); SignalWork(); return r; }

    WiFiStatus Scan() {
        const auto r = WithPlatform([&](){ return _platform.StartScan(); });
        bool selectionScan = false;
        bool provisioning = false;
        bool apRequired = false;
        uint64_t graceStarted = 0;
        if (r == WiFiStatus::Success) {
            std::lock_guard<std::mutex> lock(_mutex);
            selectionScan = UsesClient(_configuration.Mode) && _configuration.Client.Enabled &&
                _configuration.Client.Selection.AutomaticSelection && !_configuration.Client.Networks.empty() &&
                _state.Client.State != ClientState::Connected;
            provisioning = _configuration.Mode == WiFiMode::Provisioning;
            apRequired = _provisioningState.AccessPointRequired;
            graceStarted = _provisioningState.GracePeriodStartedMilliseconds;
        }
        if (selectionScan) SetSelectionState(ClientNetworkSelectionState::Scanning);
        if (selectionScan && provisioning && graceStarted == 0) {
            SetProvisioningState(ProvisioningState::ConnectingKnownNetwork, apRequired, 0);
        }
        SignalWork();
        return r;
    }

    bool AddOrUpdateClientNetwork(ClientNetworkProfile profile) {
        if (profile.SSID.empty()) return false;
        bool triggerProvisioningScan = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find_if(_configuration.Client.Networks.begin(), _configuration.Client.Networks.end(), [&](const ClientNetworkProfile& existing){ return existing.SSID == profile.SSID; });
            if (it == _configuration.Client.Networks.end()) _configuration.Client.Networks.push_back(std::move(profile));
            else *it = std::move(profile);
            triggerProvisioningScan = _configuration.Mode == WiFiMode::Provisioning &&
                _configuration.Client.Enabled && _configuration.Client.Selection.AutomaticSelection;
        }
        if (triggerProvisioningScan) (void)Scan();
        SignalWork();
        return true;
    }

    bool RemoveClientNetwork(const std::string& ssid) {
        bool removed = false;
        bool provisioningNowEmpty = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto& networks = _configuration.Client.Networks;
            const auto oldSize = networks.size();
            networks.erase(std::remove_if(networks.begin(), networks.end(), [&](const ClientNetworkProfile& p){ return p.SSID == ssid; }), networks.end());
            removed = networks.size() != oldSize;
            provisioningNowEmpty = removed && _configuration.Mode == WiFiMode::Provisioning && networks.empty();
        }
        if (provisioningNowEmpty) EnsureProvisioningAccessPoint(ProvisioningState::AccessPointAvailable);
        if (removed) SignalWork();
        return removed;
    }

    bool SetClientNetworkPriority(const std::string& ssid, uint16_t priority) {
        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            for (auto& p : _configuration.Client.Networks) {
                if (p.SSID == ssid) { p.Priority = priority; changed = true; break; }
            }
        }
        if (changed) SignalWork();
        return changed;
    }

    void OnModeChanged(ModeCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _modeCallback = std::move(cb); }
    void OnClientStateChanged(ClientCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _clientCallback = std::move(cb); }
    void OnAccessPointStateChanged(AccessPointCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _apCallback = std::move(cb); }
    void OnProvisioningStateChanged(ProvisioningCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _provisioningCallback = std::move(cb); }
    void OnScanStateChanged(ScanStateCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _scanStateCallback = std::move(cb); }
    void OnScanCompleted(ScanCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _scanCallback = std::move(cb); }
    void OnAccessPointStationConnected(StationCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _stationConnected = std::move(cb); }
    void OnAccessPointStationDisconnected(StationCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _stationDisconnected = std::move(cb); }
    void OnClientIPAddressAcquired(IPAddressCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _ipAcquired = std::move(cb); }
    void OnClientIPAddressLost(IPLostCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _ipLost = std::move(cb); }
    void OnClientNetworkSelectionChanged(SelectionCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _selectionCallback = std::move(cb); }
    void OnClientNetworkSelected(SelectedNetworkCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _selectedCallback = std::move(cb); }
    void OnClientNoKnownNetworkAvailable(SimpleCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _noKnownNetworkCallback = std::move(cb); }

    WiFiStatus ProcessOnce() {
        WiFiRuntimeState next;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            next = _state;
        }

        std::vector<ScanResult> scan;
        std::vector<WiFiPlatformEvent> events;
        const auto status = WithPlatform([&]() { return _platform.Poll(next, &scan, &events); });
        if (status != WiFiStatus::Success) return status;

        WiFiRuntimeState before;
        bool scanCompleted = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            before = _state;
            before.Client.Selection = _selectionState;
            before.Provisioning = _provisioningState;
            next.Client.Selection = _selectionState;
            next.Provisioning = _provisioningState;
            scanCompleted = before.Scan != ScanState::Complete && next.Scan == ScanState::Complete;
            _state = next;
            if (scanCompleted) _lastScanResults = scan;
        }

        NotifyStateTransitions(before, next);

        if (scanCompleted) {
            auto callback = CopyCallback(_scanCallback);
            if (callback) callback(scan);
            _observable->ScanComplete(scan);
            HandleCompletedScan(scan);
        }

        for (const auto& event : events) NotifyPlatformEvent(event);
        HandleConnectionProgress(before.Client.State, next.Client.State);
        HandleProvisioningTimeout();
        return WiFiStatus::Success;
    }

    WiFiStatus Poll() { return ProcessOnce(); }

private:
    static bool UsesClient(WiFiMode mode) {
        return mode == WiFiMode::Client ||
            mode == WiFiMode::AccessPointClient ||
            mode == WiFiMode::Provisioning;
    }

    uint64_t NowMilliseconds() const { return _clock(); }

    template<typename F>
    WiFiStatus WithPlatform(F&& operation) {
        std::lock_guard<std::mutex> lock(_platformMutex);
        return operation();
    }

    template<typename T>
    T CopyCallback(const T& callback) const {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        return callback;
    }

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
            before = _selectionState;
            _selectionState.State = state;
            after = _selectionState;
        }
        if (before.State != after.State) {
            auto callback = CopyCallback(_selectionCallback);
            if (callback) callback(before, after);
            _observable->Selection(before, after);
        }
    }

    void SetProvisioningState(
        ProvisioningState state,
        bool accessPointRequired,
        uint64_t graceStarted
    ) {
        ProvisioningRuntimeState before;
        ProvisioningRuntimeState after;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            before = _provisioningState;
            _provisioningState.State = state;
            _provisioningState.AccessPointRequired = accessPointRequired;
            _provisioningState.GracePeriodStartedMilliseconds = graceStarted;
            _provisioningState.FallbackTimeoutMilliseconds =
                _configuration.Provisioning.AccessPointFallbackTimeoutMilliseconds;
            after = _provisioningState;
        }
        if (
            before.State != after.State ||
            before.AccessPointRequired != after.AccessPointRequired ||
            before.GracePeriodStartedMilliseconds != after.GracePeriodStartedMilliseconds ||
            before.FallbackTimeoutMilliseconds != after.FallbackTimeoutMilliseconds
        ) {
            auto callback = CopyCallback(_provisioningCallback);
            if (callback) callback(before, after);
            _observable->Provisioning(before, after);
        }
    }

    void EnsureProvisioningAccessPoint(ProvisioningState state) {
        bool shouldStart = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_configuration.Mode != WiFiMode::Provisioning) return;
            shouldStart = _state.AccessPoint.State != AccessPointState::Active &&
                _state.AccessPoint.State != AccessPointState::Starting;
        }
        if (shouldStart) (void)WithPlatform([&](){ return _platform.StartAccessPoint(); });
        SetProvisioningState(state, true, 0);
        SignalWork();
    }

    void StopProvisioningAccessPointForClient() {
        bool shouldStop = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            shouldStop = _configuration.Mode == WiFiMode::Provisioning &&
                (_state.AccessPoint.State == AccessPointState::Active ||
                 _state.AccessPoint.State == AccessPointState::Starting ||
                 _provisioningState.AccessPointRequired);
        }
        if (shouldStop) (void)WithPlatform([&](){ return _platform.StopAccessPoint(); });
        SetProvisioningState(ProvisioningState::ClientConnected, false, 0);
        SignalWork();
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
        ClientState clientState;
        ProvisioningRuntimeState provisioningState;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            config = _configuration;
            clientState = _state.Client.State;
            provisioningState = _provisioningState;
        }
        if (!UsesClient(config.Mode) || !config.Client.Enabled || !config.Client.Selection.AutomaticSelection || config.Client.Networks.empty()) return;
        if (clientState == ClientState::Connected) return;

        auto candidates = BuildCandidates(scan);
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _eligibleCandidates = candidates;
            _nextCandidateIndex = 0;
            _selectionState.EligibleCandidateCount = candidates.size();
            _selectionState.SelectedSSID.clear();
            _selectionState.SelectedPriority = 0;
            _selectionState.SelectedProfileIndex = 0;
        }
        if (candidates.empty()) {
            SetSelectionState(ClientNetworkSelectionState::NoKnownNetworkAvailable);
            auto callback = CopyCallback(_noKnownNetworkCallback);
            if (callback) callback();
            _observable->NoKnownNetwork();

            if (config.Mode == WiFiMode::Provisioning) {
                if (provisioningState.GracePeriodStartedMilliseconds == 0) {
                    SetProvisioningState(
                        provisioningState.AccessPointRequired
                            ? ProvisioningState::AccessPointAvailable
                            : ProvisioningState::ConnectingKnownNetwork,
                        provisioningState.AccessPointRequired,
                        0
                    );
                }
                // Provisioning stays discoverable/recoverable and keeps looking
                // for remembered networks while its AP is available.
                (void)Scan();
            }
            return;
        }
        SetSelectionState(ClientNetworkSelectionState::Selecting);
        (void)TryNextCandidate();
    }

    WiFiStatus TryNextCandidate() {
        ClientNetworkCandidate candidate;
        ClientNetworkProfile profile;
        bool available = false;
        bool provisioning = false;
        bool apRequired = false;
        uint64_t graceStarted = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_nextCandidateIndex < _eligibleCandidates.size()) {
                candidate = _eligibleCandidates[_nextCandidateIndex++];
                if (candidate.ProfileIndex < _configuration.Client.Networks.size()) {
                    profile = _configuration.Client.Networks[candidate.ProfileIndex];
                    _selectionState.SelectedSSID = candidate.SSID;
                    _selectionState.SelectedPriority = candidate.Priority;
                    _selectionState.SelectedProfileIndex = candidate.ProfileIndex;
                    available = true;
                }
            }
            provisioning = _configuration.Mode == WiFiMode::Provisioning;
            apRequired = _provisioningState.AccessPointRequired;
            graceStarted = _provisioningState.GracePeriodStartedMilliseconds;
        }
        if (!available) {
            SetSelectionState(ClientNetworkSelectionState::Exhausted);
            return WiFiStatus::InvalidConfiguration;
        }
        auto callback = CopyCallback(_selectedCallback);
        if (callback) callback(candidate);
        _observable->Selected(candidate);
        SetSelectionState(ClientNetworkSelectionState::Connecting);
        if (provisioning) {
            SetProvisioningState(
                graceStarted != 0 ? ProvisioningState::ClientGracePeriod : ProvisioningState::ConnectingKnownNetwork,
                apRequired,
                graceStarted
            );
        }
        const auto result = WithPlatform([&]() { return _platform.ConnectClient(profile); });
        SignalWork();
        return result;
    }

    void HandleConnectionProgress(ClientState before, ClientState after) {
        WiFiConfiguration config;
        ProvisioningRuntimeState provisioningState;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            config = _configuration;
            provisioningState = _provisioningState;
        }
        if (!config.Client.Selection.AutomaticSelection || config.Client.Networks.empty()) return;

        if (after == ClientState::Connected) {
            SetSelectionState(ClientNetworkSelectionState::Connected);
            if (config.Mode == WiFiMode::Provisioning) StopProvisioningAccessPointForClient();
            return;
        }

        if (
            config.Mode == WiFiMode::Provisioning &&
            before == ClientState::Connected &&
            after != ClientState::Connected
        ) {
            const uint64_t now = NowMilliseconds();
            SetProvisioningState(ProvisioningState::ClientGracePeriod, false, now);
        }

        if (after == ClientState::Failed && before != ClientState::Failed && config.Client.Selection.TryNextOnFailure) {
            if (TryNextCandidate() == WiFiStatus::InvalidConfiguration && config.Client.Selection.ScanOnDisconnect) (void)Scan();
            return;
        }
        if (after == ClientState::Disconnected && before != ClientState::Disconnected && config.Client.Selection.ScanOnDisconnect) {
            (void)Scan();
        }
    }

    void HandleProvisioningTimeout() {
        WiFiConfiguration config;
        ProvisioningRuntimeState provisioningState;
        ClientState clientState;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            config = _configuration;
            provisioningState = _provisioningState;
            clientState = _state.Client.State;
        }

        if (config.Mode != WiFiMode::Provisioning) return;

        if (config.Client.Networks.empty()) {
            EnsureProvisioningAccessPoint(ProvisioningState::AccessPointAvailable);
            return;
        }

        if (clientState == ClientState::Connected) return;
        if (provisioningState.AccessPointRequired) return;
        if (provisioningState.GracePeriodStartedMilliseconds == 0) return;

        const uint64_t elapsed = NowMilliseconds() - provisioningState.GracePeriodStartedMilliseconds;
        if (elapsed >= config.Provisioning.AccessPointFallbackTimeoutMilliseconds) {
            bool shouldStart = false;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                shouldStart = _state.AccessPoint.State != AccessPointState::Active &&
                    _state.AccessPoint.State != AccessPointState::Starting;
            }
            if (shouldStart) (void)WithPlatform([&](){ return _platform.StartAccessPoint(); });
            SetProvisioningState(ProvisioningState::AccessPointFallback, true, 0);
            SignalWork();
        }
    }

    void NotifyStateTransitions(const WiFiRuntimeState& before, const WiFiRuntimeState& next) {
        if (before.Mode != next.Mode) {
            auto callback = CopyCallback(_modeCallback); if (callback) callback(before.Mode, next.Mode);
            _observable->Mode(before.Mode, next.Mode);
        }
        if (before.Client.State != next.Client.State || before.Client.Network.Address != next.Client.Network.Address) {
            auto callback = CopyCallback(_clientCallback); if (callback) callback(before.Client, next.Client);
            _observable->Client(before.Client, next.Client);
        }
        if (before.AccessPoint.State != next.AccessPoint.State || before.AccessPoint.ConnectedStations != next.AccessPoint.ConnectedStations) {
            auto callback = CopyCallback(_apCallback); if (callback) callback(before.AccessPoint, next.AccessPoint);
            _observable->AccessPoint(before.AccessPoint, next.AccessPoint);
        }
        if (before.Scan != next.Scan) {
            auto callback = CopyCallback(_scanStateCallback); if (callback) callback(before.Scan, next.Scan);
            _observable->ScanState(before.Scan, next.Scan);
        }
    }

    void NotifyPlatformEvent(const WiFiPlatformEvent& event) {
        switch (event.Kind) {
            case WiFiPlatformEventKind::AccessPointStationConnected: {
                auto callback = CopyCallback(_stationConnected); if (callback) callback(event.Station);
                _observable->APStationConnected(event.Station); break;
            }
            case WiFiPlatformEventKind::AccessPointStationDisconnected: {
                auto callback = CopyCallback(_stationDisconnected); if (callback) callback(event.Station);
                _observable->APStationDisconnected(event.Station); break;
            }
            case WiFiPlatformEventKind::ClientIPAddressAcquired: {
                auto callback = CopyCallback(_ipAcquired); if (callback) callback(event.Network);
                _observable->IP(event.Network); break;
            }
            case WiFiPlatformEventKind::ClientIPAddressLost: {
                auto callback = CopyCallback(_ipLost); if (callback) callback();
                _observable->IPLost(); break;
            }
        }
    }

    IWiFiPlatform& _platform;
    Clock _clock;
    mutable std::mutex _mutex;
    mutable std::mutex _callbackMutex;
    std::mutex _platformMutex;
    IWiFiConfigurationStore* _configurationStore = nullptr;
    WiFiConfiguration _configuration{};
    WiFiRuntimeState _state{};
    ClientNetworkSelectionRuntimeState _selectionState{};
    ProvisioningRuntimeState _provisioningState{};
    std::vector<ScanResult> _lastScanResults;
    std::vector<ClientNetworkCandidate> _eligibleCandidates;
    std::size_t _nextCandidateIndex = 0;
    WorkSignal _workSignal;
    std::shared_ptr<ManagerObservable> _observable = std::make_shared<ManagerObservable>();
    ModeCallback _modeCallback;
    ClientCallback _clientCallback;
    AccessPointCallback _apCallback;
    ProvisioningCallback _provisioningCallback;
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
