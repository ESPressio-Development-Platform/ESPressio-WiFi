#pragma once

#include <string>
#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

/// <summary>Outcome classification for persisted Wi-Fi configuration operations.</summary>
/**
 * ESPressio Memory Audit
 * Underlying storage: 1 bytes
 * Total Memory: 1 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
enum class WiFiConfigurationStoreStatus : uint8_t {
    Success,
    NotConfigured,
    NotFound,
    StorageError,
    SerializationError,
    ProtectionError
};

/// <summary>Result returned by Wi-Fi configuration stores.</summary>
/**
 * ESPressio Memory Audit
 * Members:
 * - Status (WiFiConfigurationStoreStatus): 1 bytes [0 bytes dynamic allocation]
 * - Message (std::string): 24 bytes [Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Total Memory: 28 bytes [Message: Capacity + 1 bytes when capacity exceeds 15-byte SSO]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * Confidence: medium; compile-time sizeof on the concrete target remains authoritative for ABI-sensitive/opaque members.
 * End ESPressio Memory Audit
 */
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
/**
 * ESPressio Memory Audit
 * Members: none; polymorphic/virtual-base object metadata is included in the total.
 * Total Memory: 4 bytes [0 bytes dynamic allocation]
 * Basis: ESP32/Xtensa ILP32 reference ABI (4-byte pointers/size_t); ESPressio stateful allocators/deleters included; ABI-sensitive STL/platform internals are identified explicitly.
 * End ESPressio Memory Audit
 */
class IWiFiConfigurationStore {
public:
    virtual ~IWiFiConfigurationStore() = default;
    /// <summary>Persists the supplied Wi-Fi configuration.</summary>
    virtual WiFiConfigurationStoreResult Save(const WiFiConfiguration& configuration) = 0;
    /// <summary>Loads persisted values into the supplied Wi-Fi configuration.</summary>
    virtual WiFiConfigurationStoreResult Load(WiFiConfiguration& configuration) = 0;
};

} // namespace ESPressio::WiFi
