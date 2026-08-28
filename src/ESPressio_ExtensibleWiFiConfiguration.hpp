#pragma once

#include <ESPressio_Serializable.hpp>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

/// <summary>Serializable wrapper that combines the standard Wi-Fi configuration with an application-defined extension object.</summary>
/// <typeparam name="TExtension">Serializable application configuration stored alongside Wi-Fi settings.</typeparam>
template<typename TExtension>
struct WiFiConfigurationWith final : Serializable::Serializable<WiFiConfigurationWith<TExtension>> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiConfigurationWith<TExtension>)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    /// <summary>Standard ESPressio Wi-Fi configuration.</summary>
    WiFiConfiguration WiFi{};
    /// <summary>Application-defined configuration extension.</summary>
    TExtension Extension{};
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("wifi", WiFi),
        ESPRESSIO_PROPERTY("extension", Extension)
    )
};

/// <summary>Owns an extensible configuration object together with the WiFiManager used to apply it.</summary>
/// <typeparam name="TConfiguration">Configuration type exposing a <c>WiFi</c> member.</typeparam>
template<typename TConfiguration>
class ConfiguredWiFiManager {
public:
    /// <summary>Constructs a configured manager over the supplied Wi-Fi platform implementation.</summary>
    explicit ConfiguredWiFiManager(IWiFiPlatform& platform) : _wifi(platform) {}
    /// <summary>Replaces the stored configuration and applies its Wi-Fi portion to the runtime manager.</summary>
    WiFiStatus Configure(TConfiguration configuration) { _configuration=std::move(configuration); return _wifi.Configure(_configuration.WiFi); }
    /// <summary>Returns mutable access to the stored configuration.</summary>
    TConfiguration& Configuration() noexcept { return _configuration; }
    /// <summary>Returns read-only access to the stored configuration.</summary>
    const TConfiguration& Configuration() const noexcept { return _configuration; }
    /// <summary>Returns mutable access to the underlying Wi-Fi runtime manager.</summary>
    WiFiManager& Runtime() noexcept { return _wifi; }
    /// <summary>Returns read-only access to the underlying Wi-Fi runtime manager.</summary>
    const WiFiManager& Runtime() const noexcept { return _wifi; }
    /// <summary>Reapplies the currently stored Wi-Fi configuration.</summary>
    WiFiStatus Apply() { return _wifi.Configure(_configuration.WiFi); }
    /// <summary>Advances Wi-Fi runtime state processing.</summary>
    WiFiStatus Poll() { return _wifi.Poll(); }
    /// <summary>Requests a Wi-Fi network scan.</summary>
    WiFiStatus Scan() { return _wifi.Scan(); }
    /// <summary>Requests client/station connection using the current configuration.</summary>
    WiFiStatus ConnectClient() { return _wifi.ConnectClient(); }
    /// <summary>Disconnects the client/station interface.</summary>
    WiFiStatus DisconnectClient() { return _wifi.DisconnectClient(); }
    /// <summary>Starts the configured access point.</summary>
    WiFiStatus StartAccessPoint() { return _wifi.StartAccessPoint(); }
    /// <summary>Stops the access point.</summary>
    WiFiStatus StopAccessPoint() { return _wifi.StopAccessPoint(); }
    /// <summary>Transitions Wi-Fi to its disabled/off state.</summary>
    WiFiStatus Disable() { return _wifi.Disable(); }
private:
    TConfiguration _configuration{};
    WiFiManager _wifi;
};

} // namespace ESPressio::WiFi
