#pragma once

#include <ESPressio_Serializable.hpp>
#include "ESPressio_WiFiConfiguration.hpp"

namespace ESPressio::WiFi {

// TExtension is any Serializable application-owned configuration type.
// The complete envelope can be persisted/protected as one Serializable object,
// while the WiFi runtime consumes only the standard `WiFi` member.
template<typename TExtension>
struct WiFiConfigurationWith final : Serializable::Serializable<WiFiConfigurationWith<TExtension>> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiConfigurationWith<TExtension>)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    WiFiConfiguration WiFi{};
    TExtension Extension{};

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("wifi", WiFi),
        ESPRESSIO_PROPERTY("extension", Extension)
    )
};

template<typename TConfiguration>
class ConfiguredWiFiManager {
public:
    explicit ConfiguredWiFiManager(IWiFiPlatform& platform) : _wifi(platform) {}

    WiFiStatus Configure(TConfiguration configuration) {
        _configuration = std::move(configuration);
        return _wifi.Configure(_configuration.WiFi);
    }

    TConfiguration& Configuration() noexcept { return _configuration; }
    const TConfiguration& Configuration() const noexcept { return _configuration; }
    WiFiManager& Runtime() noexcept { return _wifi; }
    const WiFiManager& Runtime() const noexcept { return _wifi; }

    WiFiStatus Apply() { return _wifi.Configure(_configuration.WiFi); }
    WiFiStatus Poll() { return _wifi.Poll(); }
    WiFiStatus Scan() { return _wifi.Scan(); }
    WiFiStatus ConnectClient() { return _wifi.ConnectClient(); }
    WiFiStatus DisconnectClient() { return _wifi.DisconnectClient(); }
    WiFiStatus StartAccessPoint() { return _wifi.StartAccessPoint(); }
    WiFiStatus StopAccessPoint() { return _wifi.StopAccessPoint(); }
    WiFiStatus Disable() { return _wifi.Disable(); }

private:
    TConfiguration _configuration{};
    WiFiManager _wifi;
};

} // namespace ESPressio::WiFi
