#pragma once

#include <functional>
#include <vector>

#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

enum class WiFiStatus : uint8_t {
    Success,
    InvalidConfiguration,
    NotSupported,
    Busy,
    PlatformError
};

class IWiFiPlatform {
public:
    virtual ~IWiFiPlatform() = default;
    virtual WiFiStatus Apply(const WiFiConfiguration& configuration) = 0;
    virtual WiFiStatus Disable() = 0;
    virtual WiFiStatus ConnectClient() = 0;
    virtual WiFiStatus DisconnectClient() = 0;
    virtual WiFiStatus StartAccessPoint() = 0;
    virtual WiFiStatus StopAccessPoint() = 0;
    virtual WiFiStatus StartScan() = 0;
    virtual WiFiStatus Poll(WiFiRuntimeState& state, std::vector<ScanResult>* completedScan) = 0;
};

class WiFiManager {
public:
    using StateCallback = std::function<void(const WiFiRuntimeState&, const WiFiRuntimeState&)>;
    using ScanCallback = std::function<void(const std::vector<ScanResult>&)>;

    explicit WiFiManager(IWiFiPlatform& platform) : _platform(platform) {}

    const WiFiConfiguration& Configuration() const noexcept { return _configuration; }
    const WiFiRuntimeState& State() const noexcept { return _state; }

    WiFiStatus Configure(WiFiConfiguration configuration) {
        _configuration = std::move(configuration);
        return _platform.Apply(_configuration);
    }

    WiFiStatus Disable() { return _platform.Disable(); }
    WiFiStatus ConnectClient() { return _platform.ConnectClient(); }
    WiFiStatus DisconnectClient() { return _platform.DisconnectClient(); }
    WiFiStatus StartAccessPoint() { return _platform.StartAccessPoint(); }
    WiFiStatus StopAccessPoint() { return _platform.StopAccessPoint(); }
    WiFiStatus Scan() { return _platform.StartScan(); }

    void OnStateChanged(StateCallback callback) { _stateCallback = std::move(callback); }
    void OnScanCompleted(ScanCallback callback) { _scanCallback = std::move(callback); }

    WiFiStatus Poll() {
        WiFiRuntimeState next = _state;
        std::vector<ScanResult> scan;
        const auto result = _platform.Poll(next, &scan);
        if (result != WiFiStatus::Success) return result;

        if (next.Revision != _state.Revision) {
            const auto previous = _state;
            _state = std::move(next);
            if (_stateCallback) _stateCallback(previous, _state);
        }
        if (!scan.empty() && _scanCallback) _scanCallback(scan);
        return WiFiStatus::Success;
    }

private:
    IWiFiPlatform& _platform;
    WiFiConfiguration _configuration{};
    WiFiRuntimeState _state{};
    StateCallback _stateCallback;
    ScanCallback _scanCallback;
};

} // namespace ESPressio::WiFi
