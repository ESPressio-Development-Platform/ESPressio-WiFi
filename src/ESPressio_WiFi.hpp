#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <ESPressio_Memory.hpp>
#include <ESPressio_ThreadSafeObservable.hpp>
#include "ESPressio_IWiFiObserver.hpp"
#include "ESPressio_IWiFiRadioObserver.hpp"
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
    virtual WiFiRadioState GetRadioState() const { return WiFiRadioState{}; }
};

class WiFiManager {
private:
    using ScanStorage = System::Memory::Vector<
        ScanResult,
        System::Memory::MemoryPolicy::ExternalPreferred
    >;
    using CandidateStorage = System::Memory::Vector<
        ClientNetworkCandidate,
        System::Memory::MemoryPolicy::ExternalPreferred
    >;

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
        void APUntilClient(const APUntilClientRuntimeState& before, const APUntilClientRuntimeState& after) { Notify([&](IWiFiObserver* o){ o->OnAPUntilClientStateChanged(before, after); }); }
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

    class RadioObservable final : public Observable::ThreadSafeObservable {
        template<typename F> void Notify(F&& callback) {
            ExecuteNotification([&](NotificationContext& n) {
                n.WithObservers<IWiFiRadioObserver>([&](IWiFiRadioObserver* o) {
                    try { callback(o); } catch (...) {}
                });
            });
        }
    public:
        void TransitionBeginning(const WiFiRadioState& before, WiFiRadioTransitionReason reason) {
            Notify([&](IWiFiRadioObserver* o){ o->OnWiFiRadioTransitionBeginning(before, reason); });
        }
        void TransitionCompleted(const WiFiRadioState& before, const WiFiRadioState& after, WiFiRadioTransitionReason reason) {
            Notify([&](IWiFiRadioObserver* o){ o->OnWiFiRadioTransitionCompleted(before, after, reason); });
        }
        void StateChanged(const WiFiRadioState& before, const WiFiRadioState& after) {
            Notify([&](IWiFiRadioObserver* o){ o->OnWiFiRadioStateChanged(before, after); });
        }
        void ScanBeginning(const WiFiRadioState& before) { Notify([&](IWiFiRadioObserver* o){ o->OnWiFiRadioScanBeginning(before); }); }
        void ScanCompleted(const WiFiRadioState& after) { Notify([&](IWiFiRadioObserver* o){ o->OnWiFiRadioScanCompleted(after); }); }
    };

public:
    using Clock = std::function<uint64_t()>;
    using ModeCallback = std::function<void(WiFiMode, WiFiMode)>;
    using ClientCallback = std::function<void(const ClientRuntimeState&, const ClientRuntimeState&)>;
    using AccessPointCallback = std::function<void(const AccessPointRuntimeState&, const AccessPointRuntimeState&)>;
    using APUntilClientCallback = std::function<void(const APUntilClientRuntimeState&, const APUntilClientRuntimeState&)>;
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
                      std::chrono::steady_clock::now().time_since_epoch()).count());
          })),
          _observable(System::Memory::MakeShared<ManagerObservable, System::Memory::MemoryPolicy::ExternalPreferred>()),
          _radioObservable(System::Memory::MakeShared<RadioObservable, System::Memory::MemoryPolicy::ExternalPreferred>()) {}

    WiFiConfiguration Configuration() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _configuration;
    }

    template<typename Callback>
    decltype(auto) WithConfiguration(Callback&& callback) const {
        std::lock_guard<std::mutex> lock(_mutex);
        return std::forward<Callback>(callback)(_configuration);
    }

    WiFiRuntimeState State() const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto state = _state;
        state.Client.Selection = _selectionState;
        state.APUntilClient = _apUntilClientState;
        return state;
    }

    WiFiRadioState RadioState() const { return ReadRadioState(); }

    std::vector<ScanResult> LastScanResults() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return std::vector<ScanResult>(_lastScanResults.begin(), _lastScanResults.end());
    }

    std::vector<ClientNetworkCandidate> EligibleClientNetworks() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return std::vector<ClientNetworkCandidate>(_eligibleCandidates.begin(), _eligibleCandidates.end());
    }

    void SetWorkSignal(WorkSignal signal) { std::lock_guard<std::mutex> lock(_mutex); _workSignal = std::move(signal); }
    void SetConfigurationStore(IWiFiConfigurationStore* store) { std::lock_guard<std::mutex> lock(_mutex); _configurationStore = store; }
    IWiFiConfigurationStore* ConfigurationStore() const { std::lock_guard<std::mutex> lock(_mutex); return _configurationStore; }

    WiFiConfigurationStoreResult SaveConfiguration() {
        IWiFiConfigurationStore* store = nullptr;
        WiFiConfiguration snapshot;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            store = _configurationStore;
            snapshot = _configuration; // independent snapshot required across storage call
        }
        if (!store) return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::NotConfigured, "No WiFi configuration store is configured");
        return store->Save(snapshot);
    }

    WiFiConfigurationStoreResult LoadConfiguration(bool apply = true) {
        IWiFiConfigurationStore* store = nullptr;
        { std::lock_guard<std::mutex> lock(_mutex); store = _configurationStore; }
        if (!store) return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::NotConfigured, "No WiFi configuration store is configured");
        WiFiConfiguration loaded;
        auto result = store->Load(loaded);
        if (!result) return result;
        if (apply) {
            if (Configure(std::move(loaded)) != WiFiStatus::Success)
                return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::StorageError, "Configuration loaded but could not be applied to the WiFi platform");
        } else {
            std::lock_guard<std::mutex> lock(_mutex);
            _configuration = std::move(loaded);
        }
        SignalWork();
        return result;
    }

    Observable::ObserverHandlePtr RegisterObserver(IWiFiObserver* observer) { return _observable->RegisterObserver(observer); }
    void UnregisterObserver(IWiFiObserver* observer) { _observable->UnregisterObserver(observer); }
    Observable::ObserverHandlePtr RegisterRadioObserver(IWiFiRadioObserver* observer) { return _radioObservable->RegisterObserver(observer); }
    void UnregisterRadioObserver(IWiFiRadioObserver* observer) { _radioObservable->UnregisterObserver(observer); }

    WiFiStatus Configure(WiFiConfiguration configuration) {
        std::lock_guard<std::recursive_mutex> transitionLock(_radioTransitionMutex);
        const auto status = ExecuteRadioTransition(
            WiFiRadioTransitionReason::Configuration,
            [&](){ return _platform.Apply(configuration); }
        );
        if (status != WiFiStatus::Success) return status;

        bool scanOnStartup = false;
        bool immediateFallback = false;
        bool seeking = false;
        uint64_t fallbackDeadline = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _configuration = std::move(configuration);
            _eligibleCandidates.clear();
            _nextCandidateIndex = 0;
            _selectionState = {};
            _apUntilClientState = {};
            const bool hasRemembered = !_configuration.Client.Networks.empty();
            if (_configuration.Mode == WiFiMode::APUntilClient) {
                immediateFallback = !hasRemembered;
                seeking = hasRemembered;
                if (hasRemembered) fallbackDeadline = NowMilliseconds() + _configuration.APUntilClient.FallbackTimeoutMilliseconds;
            }
            scanOnStartup = UsesClient(_configuration.Mode) && _configuration.Client.Enabled && hasRemembered &&
                _configuration.Client.Selection.AutomaticSelection && _configuration.Client.Selection.ScanOnStartup;
        }
        if (seeking) SetAPUntilClientState(APUntilClientState::SeekingClient, false, fallbackDeadline, 0);
        if (immediateFallback) (void)ActivateFallbackAccessPoint();
        else if (scanOnStartup) (void)Scan();
        SignalWork();
        return status;
    }

    WiFiStatus ApplyConfiguration() { return Configure(Configuration()); }
    WiFiStatus Disable() {
        auto configuration = Configuration();
        configuration.Mode = WiFiMode::Off;
        return Configure(std::move(configuration));
    }
    WiFiStatus ConnectClient() {
        const auto r = ExecuteRadioTransition(WiFiRadioTransitionReason::ClientConnect, [&](){ return _platform.ConnectClient(); });
        SignalWork(); return r;
    }
    WiFiStatus DisconnectClient() {
        const auto r = ExecuteRadioTransition(WiFiRadioTransitionReason::ClientDisconnect, [&](){ return _platform.DisconnectClient(); });
        SignalWork(); return r;
    }
    WiFiStatus StartAccessPoint() {
        const auto r = ExecuteRadioTransition(WiFiRadioTransitionReason::AccessPointStart, [&](){ return _platform.StartAccessPoint(); });
        SignalWork(); return r;
    }
    WiFiStatus StopAccessPoint() {
        const auto r = ExecuteRadioTransition(WiFiRadioTransitionReason::AccessPointStop, [&](){ return _platform.StopAccessPoint(); });
        SignalWork(); return r;
    }

    WiFiStatus Scan() {
        std::lock_guard<std::recursive_mutex> transitionLock(_radioTransitionMutex);
        const auto before = ReadRadioState();
        _radioObservable->ScanBeginning(before);
        const auto r = ExecuteRadioTransition(WiFiRadioTransitionReason::Scan, [&](){ return _platform.StartScan(); });
        bool selectionScan = false;
        if (r == WiFiStatus::Success) {
            std::lock_guard<std::mutex> lock(_mutex);
            selectionScan = UsesClient(_configuration.Mode) && _configuration.Client.Enabled &&
                _configuration.Client.Selection.AutomaticSelection && !_configuration.Client.Networks.empty() &&
                _state.Client.State != ClientState::Connected;
        } else {
            _radioObservable->ScanCompleted(ReadRadioState());
        }
        if (selectionScan) SetSelectionState(ClientNetworkSelectionState::Scanning);
        SignalWork();
        return r;
    }

    WiFiStatus RetryKnownNetworksNow() {
        bool valid = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            valid = _configuration.Mode == WiFiMode::APUntilClient &&
                _configuration.Client.Enabled &&
                _configuration.Client.Selection.AutomaticSelection &&
                !_configuration.Client.Networks.empty();
        }
        return valid ? Scan() : WiFiStatus::InvalidConfiguration;
    }

    bool AddOrUpdateClientNetwork(ClientNetworkProfile profile) {
        if (profile.SSID.empty()) return false;
        bool retry = false;
        bool startSeeking = false;
        uint64_t deadline = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = std::find_if(_configuration.Client.Networks.begin(), _configuration.Client.Networks.end(),
                [&](const ClientNetworkProfile& p){ return p.SSID == profile.SSID; });
            if (it == _configuration.Client.Networks.end()) _configuration.Client.Networks.push_back(std::move(profile));
            else *it = std::move(profile);
            retry = _configuration.Mode == WiFiMode::APUntilClient && _configuration.Client.Enabled &&
                _configuration.Client.Selection.AutomaticSelection;
            if (_configuration.Mode == WiFiMode::APUntilClient && !_apUntilClientState.FallbackAccessPointActive &&
                _apUntilClientState.FallbackDeadlineMilliseconds == 0) {
                deadline = NowMilliseconds() + _configuration.APUntilClient.FallbackTimeoutMilliseconds;
                startSeeking = true;
            }
        }
        if (startSeeking) SetAPUntilClientState(APUntilClientState::SeekingClient, false, deadline, 0);
        if (retry) (void)Scan();
        SignalWork();
        return true;
    }

    bool RemoveClientNetwork(const std::string& ssid) {
        bool removed = false;
        bool immediateFallback = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            auto& networks = _configuration.Client.Networks;
            const auto old = networks.size();
            networks.erase(std::remove_if(networks.begin(), networks.end(),
                [&](const ClientNetworkProfile& p){ return p.SSID == ssid; }), networks.end());
            removed = networks.size() != old;
            immediateFallback = removed && _configuration.Mode == WiFiMode::APUntilClient && networks.empty();
        }
        if (immediateFallback) (void)ActivateFallbackAccessPoint();
        if (removed) SignalWork();
        return removed;
    }

    bool SetClientNetworkPriority(const std::string& ssid, uint16_t priority) {
        bool changed = false;
        { std::lock_guard<std::mutex> lock(_mutex); for (auto& p : _configuration.Client.Networks) if (p.SSID == ssid) { p.Priority = priority; changed = true; break; } }
        if (changed) SignalWork();
        return changed;
    }

    void OnModeChanged(ModeCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _modeCallback = std::move(cb); }
    void OnClientStateChanged(ClientCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _clientCallback = std::move(cb); }
    void OnAccessPointStateChanged(AccessPointCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _apCallback = std::move(cb); }
    void OnAPUntilClientStateChanged(APUntilClientCallback cb) { std::lock_guard<std::mutex> lock(_callbackMutex); _apUntilClientCallback = std::move(cb); }
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
        std::lock_guard<std::recursive_mutex> transitionLock(_radioTransitionMutex);
        WiFiRuntimeState next;
        { std::lock_guard<std::mutex> lock(_mutex); next = _state; }
        std::vector<ScanResult> scan;
        std::vector<WiFiPlatformEvent> events;
        WiFiRadioState radioAfter;
        WiFiStatus status = WiFiStatus::PlatformError;
        {
            std::lock_guard<std::mutex> lock(_platformMutex);
            status = _platform.Poll(next, &scan, &events);
            radioAfter = _platform.GetRadioState();
        }
        PublishRadioState(radioAfter);
        if (status != WiFiStatus::Success) return status;

        WiFiRuntimeState before;
        bool scanCompleted = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            before = _state;
            before.Client.Selection = _selectionState;
            before.APUntilClient = _apUntilClientState;
            next.Client.Selection = _selectionState;
            next.APUntilClient = _apUntilClientState;
            scanCompleted = before.Scan != ScanState::Complete && next.Scan == ScanState::Complete;
            _state = next;
            if (scanCompleted) {
                _lastScanResults.clear();
                _lastScanResults.reserve(scan.size());
                _lastScanResults.insert(_lastScanResults.end(), scan.begin(), scan.end());
            }
        }
        NotifyStateTransitions(before, next);
        if (scanCompleted) {
            auto cb = CopyCallback(_scanCallback);
            if (cb) cb(scan);
            _observable->ScanComplete(scan);
            _radioObservable->ScanCompleted(radioAfter);
            HandleCompletedScan(scan);
        }
        for (const auto& event : events) NotifyPlatformEvent(event);
        HandleConnectionProgress(before.Client.State, next.Client.State);
        HandleAPUntilClientTimers();
        return WiFiStatus::Success;
    }

    WiFiStatus Poll() { return ProcessOnce(); }

private:
    static bool UsesClient(WiFiMode mode) {
        return mode == WiFiMode::Client || mode == WiFiMode::AccessPointClient || mode == WiFiMode::APUntilClient;
    }
    uint64_t NowMilliseconds() const { return _clock(); }
    template<typename F> WiFiStatus WithPlatform(F&& op) { std::lock_guard<std::mutex> lock(_platformMutex); return op(); }
    template<typename T> T CopyCallback(const T& cb) const { std::lock_guard<std::mutex> lock(_callbackMutex); return cb; }
    void SignalWork() { WorkSignal signal; { std::lock_guard<std::mutex> lock(_mutex); signal = _workSignal; } if (signal) signal(); }

    WiFiRadioState ReadRadioState() const {
        std::lock_guard<std::mutex> lock(_platformMutex);
        return _platform.GetRadioState();
    }

    void PublishRadioState(const WiFiRadioState& after) {
        WiFiRadioState before;
        bool changed = false;
        {
            std::lock_guard<std::mutex> lock(_radioStateMutex);
            before = _lastRadioState;
            changed = !_haveRadioState || before != after;
            _lastRadioState = after;
            _haveRadioState = true;
        }
        if (changed) _radioObservable->StateChanged(before, after);
    }

    template<typename F>
    WiFiStatus ExecuteRadioTransition(WiFiRadioTransitionReason reason, F&& op) {
        std::lock_guard<std::recursive_mutex> transitionLock(_radioTransitionMutex);
        const WiFiRadioState before = ReadRadioState();
        _radioObservable->TransitionBeginning(before, reason);
        WiFiStatus status = WiFiStatus::PlatformError;
        WiFiRadioState after;
        {
            std::lock_guard<std::mutex> lock(_platformMutex);
            status = op();
            after = _platform.GetRadioState();
        }
        _radioObservable->TransitionCompleted(before, after, reason);
        PublishRadioState(after);
        return status;
    }

    void SetSelectionState(ClientNetworkSelectionState state) {
        ClientNetworkSelectionRuntimeState before, after;
        { std::lock_guard<std::mutex> lock(_mutex); before = _selectionState; _selectionState.State = state; after = _selectionState; }
        if (before.State != after.State) {
            auto cb = CopyCallback(_selectionCallback); if (cb) cb(before, after);
            _observable->Selection(before, after);
        }
    }

    void SetAPUntilClientState(APUntilClientState state, bool fallbackActive, uint64_t deadline, uint64_t nextRetry) {
        APUntilClientRuntimeState before, after;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            before = _apUntilClientState;
            _apUntilClientState.State = state;
            _apUntilClientState.FallbackAccessPointActive = fallbackActive;
            _apUntilClientState.FallbackDeadlineMilliseconds = deadline;
            _apUntilClientState.NextRetryMilliseconds = nextRetry;
            after = _apUntilClientState;
        }
        if (before.State != after.State || before.FallbackAccessPointActive != after.FallbackAccessPointActive ||
            before.FallbackDeadlineMilliseconds != after.FallbackDeadlineMilliseconds || before.NextRetryMilliseconds != after.NextRetryMilliseconds) {
            auto cb = CopyCallback(_apUntilClientCallback); if (cb) cb(before, after);
            _observable->APUntilClient(before, after);
        }
    }

    CandidateStorage BuildCandidates(const std::vector<ScanResult>& scan) const {
        CandidateStorage candidates;
        std::lock_guard<std::mutex> lock(_mutex);
        candidates.reserve(_configuration.Client.Networks.size());
        for (std::size_t i = 0; i < _configuration.Client.Networks.size(); ++i) {
            const auto& p = _configuration.Client.Networks[i];
            if (!p.Enabled || p.SSID.empty()) continue;
            const ScanResult* strongest = nullptr;
            for (const auto& visible : scan) {
                if (visible.SSID == p.SSID && (!strongest || visible.RSSI > strongest->RSSI)) strongest = &visible;
            }
            if (!strongest) continue;
            ClientNetworkCandidate c;
            c.SSID = p.SSID;
            c.BSSID = strongest->BSSID;
            c.Priority = p.Priority;
            c.RSSI = strongest->RSSI;
            c.Channel = strongest->Channel;
            c.ProfileIndex = i;
            candidates.push_back(std::move(c));
        }
        std::sort(candidates.begin(), candidates.end(), [](const auto& a, const auto& b) {
            if (a.Priority != b.Priority) return a.Priority > b.Priority;
            if (a.RSSI != b.RSSI) return a.RSSI > b.RSSI;
            return a.SSID < b.SSID;
        });
        return candidates;
    }

    void HandleCompletedScan(const std::vector<ScanResult>& scan) {
        bool shouldSelect = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            shouldSelect = UsesClient(_configuration.Mode) && _configuration.Client.Enabled &&
                _configuration.Client.Selection.AutomaticSelection && !_configuration.Client.Networks.empty() &&
                _state.Client.State != ClientState::Connected;
        }
        if (!shouldSelect) return;

        auto candidates = BuildCandidates(scan);
        bool noCandidates = false;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _eligibleCandidates = std::move(candidates);
            _nextCandidateIndex = 0;
            _selectionState.EligibleCandidateCount = _eligibleCandidates.size();
            _selectionState.SelectedSSID.clear();
            _selectionState.SelectedPriority = 0;
            _selectionState.SelectedProfileIndex = 0;
            noCandidates = _eligibleCandidates.empty();
        }
        if (noCandidates) {
            SetSelectionState(ClientNetworkSelectionState::NoKnownNetworkAvailable);
            auto cb = CopyCallback(_noKnownNetworkCallback); if (cb) cb();
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
                    profile = _configuration.Client.Networks[candidate.ProfileIndex]; // independent platform-call snapshot
                    _selectionState.SelectedSSID = candidate.SSID;
                    _selectionState.SelectedPriority = candidate.Priority;
                    _selectionState.SelectedProfileIndex = candidate.ProfileIndex;
                    available = true;
                }
            }
        }
        if (!available) { SetSelectionState(ClientNetworkSelectionState::Exhausted); return WiFiStatus::InvalidConfiguration; }
        auto cb = CopyCallback(_selectedCallback); if (cb) cb(candidate);
        _observable->Selected(candidate);
        SetSelectionState(ClientNetworkSelectionState::Connecting);
        const auto result = ExecuteRadioTransition(
            WiFiRadioTransitionReason::ClientConnect,
            [&](){ return _platform.ConnectClient(profile); }
        );
        SignalWork();
        return result;
    }

    WiFiStatus ActivateFallbackAccessPoint() {
        bool alreadyActive = false;
        bool networksEmpty = true;
        uint32_t retryInterval = 0;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            alreadyActive = _apUntilClientState.FallbackAccessPointActive;
            networksEmpty = _configuration.Client.Networks.empty();
            retryInterval = _configuration.APUntilClient.RetryScanIntervalMilliseconds;
        }
        if (alreadyActive) return WiFiStatus::Success;
        const auto result = ExecuteRadioTransition(
            WiFiRadioTransitionReason::AccessPointStart,
            [&](){ return _platform.StartAccessPoint(); }
        );
        if (result == WiFiStatus::Success) {
            const auto nextRetry = networksEmpty ? 0 : NowMilliseconds() + retryInterval;
            SetAPUntilClientState(APUntilClientState::FallbackAccessPoint, true, 0, nextRetry);
        }
        SignalWork();
        return result;
    }

    void ArmFallbackAfterClientLoss() {
        WiFiMode mode;
        bool networksEmpty;
        uint32_t timeout;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            mode = _configuration.Mode;
            networksEmpty = _configuration.Client.Networks.empty();
            timeout = _configuration.APUntilClient.FallbackTimeoutMilliseconds;
        }
        if (mode != WiFiMode::APUntilClient) return;
        if (networksEmpty) { (void)ActivateFallbackAccessPoint(); return; }
        SetAPUntilClientState(APUntilClientState::SeekingClient, false, NowMilliseconds() + timeout, 0);
    }

    void HandleAPUntilClientTimers() {
        WiFiMode mode;
        bool networksEmpty;
        uint32_t retryInterval;
        APUntilClientRuntimeState lifecycle;
        ClientState clientState;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            mode = _configuration.Mode;
            networksEmpty = _configuration.Client.Networks.empty();
            retryInterval = _configuration.APUntilClient.RetryScanIntervalMilliseconds;
            lifecycle = _apUntilClientState;
            clientState = _state.Client.State;
        }
        if (mode != WiFiMode::APUntilClient) return;
        const auto now = NowMilliseconds();
        if (clientState == ClientState::Connected) {
            if (lifecycle.FallbackAccessPointActive)
                (void)ExecuteRadioTransition(WiFiRadioTransitionReason::AccessPointStop, [&](){ return _platform.StopAccessPoint(); });
            if (lifecycle.State != APUntilClientState::ClientConnected || lifecycle.FallbackAccessPointActive)
                SetAPUntilClientState(APUntilClientState::ClientConnected, false, 0, 0);
            return;
        }
        if (!lifecycle.FallbackAccessPointActive && (networksEmpty ||
            (lifecycle.FallbackDeadlineMilliseconds != 0 && now >= lifecycle.FallbackDeadlineMilliseconds))) {
            (void)ActivateFallbackAccessPoint();
            return;
        }
        if (lifecycle.FallbackAccessPointActive && !networksEmpty &&
            lifecycle.NextRetryMilliseconds != 0 && now >= lifecycle.NextRetryMilliseconds) {
            SetAPUntilClientState(APUntilClientState::FallbackAccessPoint, true, 0, now + retryInterval);
            (void)Scan();
        }
    }

    void HandleConnectionProgress(ClientState before, ClientState after) {
        WiFiMode mode;
        bool automatic;
        bool networksEmpty;
        bool tryNext;
        bool scanOnDisconnect;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            mode = _configuration.Mode;
            automatic = _configuration.Client.Selection.AutomaticSelection;
            networksEmpty = _configuration.Client.Networks.empty();
            tryNext = _configuration.Client.Selection.TryNextOnFailure;
            scanOnDisconnect = _configuration.Client.Selection.ScanOnDisconnect;
        }
        if (mode == WiFiMode::APUntilClient && before == ClientState::Connected && after != ClientState::Connected) {
            ArmFallbackAfterClientLoss();
        }
        if (!automatic || networksEmpty) return;
        if (after == ClientState::Connected) {
            SetSelectionState(ClientNetworkSelectionState::Connected);
            HandleAPUntilClientTimers();
            return;
        }
        if (after == ClientState::Failed && before != ClientState::Failed && tryNext) {
            if (TryNextCandidate() == WiFiStatus::InvalidConfiguration && scanOnDisconnect) (void)Scan();
            return;
        }
        if (after == ClientState::Disconnected && before != ClientState::Disconnected && scanOnDisconnect) (void)Scan();
    }

    void NotifyStateTransitions(const WiFiRuntimeState& before, const WiFiRuntimeState& next) {
        if (before.Mode != next.Mode) { auto cb = CopyCallback(_modeCallback); if (cb) cb(before.Mode, next.Mode); _observable->Mode(before.Mode, next.Mode); }
        if (before.Client.State != next.Client.State || before.Client.Network.Address != next.Client.Network.Address) { auto cb = CopyCallback(_clientCallback); if (cb) cb(before.Client, next.Client); _observable->Client(before.Client, next.Client); }
        if (before.AccessPoint.State != next.AccessPoint.State || before.AccessPoint.ConnectedStations != next.AccessPoint.ConnectedStations) { auto cb = CopyCallback(_apCallback); if (cb) cb(before.AccessPoint, next.AccessPoint); _observable->AccessPoint(before.AccessPoint, next.AccessPoint); }
        if (before.Scan != next.Scan) { auto cb = CopyCallback(_scanStateCallback); if (cb) cb(before.Scan, next.Scan); _observable->ScanState(before.Scan, next.Scan); }
    }

    void NotifyPlatformEvent(const WiFiPlatformEvent& event) {
        switch (event.Kind) {
            case WiFiPlatformEventKind::AccessPointStationConnected: { auto cb = CopyCallback(_stationConnected); if (cb) cb(event.Station); _observable->APStationConnected(event.Station); break; }
            case WiFiPlatformEventKind::AccessPointStationDisconnected: { auto cb = CopyCallback(_stationDisconnected); if (cb) cb(event.Station); _observable->APStationDisconnected(event.Station); break; }
            case WiFiPlatformEventKind::ClientIPAddressAcquired: { auto cb = CopyCallback(_ipAcquired); if (cb) cb(event.Network); _observable->IP(event.Network); break; }
            case WiFiPlatformEventKind::ClientIPAddressLost: { auto cb = CopyCallback(_ipLost); if (cb) cb(); _observable->IPLost(); break; }
        }
    }

    IWiFiPlatform& _platform;
    Clock _clock;
    mutable std::mutex _mutex;
    mutable std::mutex _callbackMutex;
    mutable std::mutex _platformMutex;
    mutable std::recursive_mutex _radioTransitionMutex;
    mutable std::mutex _radioStateMutex;
    IWiFiConfigurationStore* _configurationStore = nullptr;
    WiFiConfiguration _configuration{}; // Serializable public schema remains unchanged.
    WiFiRuntimeState _state{};
    ClientNetworkSelectionRuntimeState _selectionState{};
    APUntilClientRuntimeState _apUntilClientState{};
    ScanStorage _lastScanResults;
    CandidateStorage _eligibleCandidates;
    std::size_t _nextCandidateIndex = 0;
    WorkSignal _workSignal;
    std::shared_ptr<ManagerObservable> _observable;
    std::shared_ptr<RadioObservable> _radioObservable;
    WiFiRadioState _lastRadioState{};
    bool _haveRadioState = false;
    ModeCallback _modeCallback;
    ClientCallback _clientCallback;
    AccessPointCallback _apCallback;
    APUntilClientCallback _apUntilClientCallback;
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
