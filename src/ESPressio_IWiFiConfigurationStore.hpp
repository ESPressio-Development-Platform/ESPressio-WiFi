#pragma once

#include <string>
#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

/// <summary>Outcome classification for persisted Wi-Fi configuration operations.</summary>
enum class WiFiConfigurationStoreStatus : uint8_t {
    Success,
    NotConfigured,
    NotFound,
    StorageError,
    SerializationError,
    ProtectionError
};

/// <summary>Result returned by Wi-Fi configuration stores.</summary>
struct WiFiConfigurationStoreResult {
    /// <summary>Operation outcome.</summary>
    WiFiConfigurationStoreStatus Status = WiFiConfigurationStoreStatus::Success;
    /// <summary>Optional diagnostic message describing a failure.</summary>
    std::string Message;

    /// <summary>Indicates whether the store operation completed successfully.</summary>
    bool Success() const noexcept { return Status == WiFiConfigurationStoreStatus::Success; }
    explicit operator bool() const noexcept { return Success(); }

    /// <summary>Creates a successful result.</summary>
    static WiFiConfigurationStoreResult Ok() { return {}; }
    /// <summary>Creates a failed result with an optional diagnostic message.</summary>
    static WiFiConfigurationStoreResult Fail(WiFiConfigurationStoreStatus status, std::string message = {}) {
        return {status, std::move(message)};
    }
};

/// <summary>Persistence contract for saving and loading complete Wi-Fi configurations.</summary>
class IWiFiConfigurationStore {
public:
    virtual ~IWiFiConfigurationStore() = default;
    /// <summary>Persists the supplied Wi-Fi configuration.</summary>
    virtual WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) = 0;
    /// <summary>Loads persisted values into the supplied Wi-Fi configuration.</summary>
    virtual WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) = 0;
};

} // namespace ESPressio::WiFi
