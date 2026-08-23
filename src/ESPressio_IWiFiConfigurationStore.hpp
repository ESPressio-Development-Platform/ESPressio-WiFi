#pragma once

#include <string>
#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

enum class WiFiConfigurationStoreStatus : uint8_t {
    Success,
    NotConfigured,
    NotFound,
    StorageError,
    SerializationError,
    ProtectionError
};

struct WiFiConfigurationStoreResult {
    WiFiConfigurationStoreStatus Status = WiFiConfigurationStoreStatus::Success;
    std::string Message;

    bool Success() const noexcept { return Status == WiFiConfigurationStoreStatus::Success; }
    explicit operator bool() const noexcept { return Success(); }

    static WiFiConfigurationStoreResult Ok() { return {}; }
    static WiFiConfigurationStoreResult Fail(WiFiConfigurationStoreStatus status, std::string message = {}) {
        return {status, std::move(message)};
    }
};

class IWiFiConfigurationStore {
public:
    virtual ~IWiFiConfigurationStore() = default;
    virtual WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) = 0;
    virtual WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) = 0;
};

} // namespace ESPressio::WiFi
