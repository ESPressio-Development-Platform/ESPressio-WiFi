#pragma once

#include <cstdint>

#include <ESPressio_Serializable.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

/// <summary>Persistable client-network profile considered during automatic network selection.</summary>
struct ClientNetworkProfile final : Serializable::Serializable<ClientNetworkProfile> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientNetworkProfile)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ClientNetworkProfile() = default;

    /// <summary>Network SSID retained in externally preferred storage.</summary>
    WiFiString SSID;
    /// <summary>Network password retained in externally preferred storage; serialized as a sensitive property.</summary>
    WiFiString Password;
    /// <summary>Selection priority; lower/higher ordering is interpreted by the Wi-Fi selection implementation.</summary>
    uint16_t Priority = 100;
    /// <summary>Whether this profile is eligible for automatic selection.</summary>
    bool Enabled = true;
    /// <summary>Address assignment mode used after association.</summary>
    AddressMode Addressing = AddressMode::DHCP;
    /// <summary>Static network values used when static addressing is selected.</summary>
    NetworkAddress StaticNetwork{};

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY_SENSITIVE("password", Password),
        ESPRESSIO_PROPERTY("priority", Priority),
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("addressing", Addressing),
        ESPRESSIO_PROPERTY("staticNetwork", StaticNetwork)
    )
};

/// <summary>Controls automatic selection and retry behavior across known client networks.</summary>
struct ClientNetworkSelectionConfiguration final
    : Serializable::Serializable<ClientNetworkSelectionConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientNetworkSelectionConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ClientNetworkSelectionConfiguration() = default;

    /// <summary>Whether the worker may automatically select among configured known networks.</summary>
    bool AutomaticSelection = true;
    /// <summary>Whether a scan is performed when client operation starts.</summary>
    bool ScanOnStartup = true;
    /// <summary>Whether a scan is performed after a client disconnect.</summary>
    bool ScanOnDisconnect = true;
    /// <summary>Whether connection failure advances to another known candidate.</summary>
    bool TryNextOnFailure = true;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("automaticSelection", AutomaticSelection),
        ESPRESSIO_PROPERTY("scanOnStartup", ScanOnStartup),
        ESPRESSIO_PROPERTY("scanOnDisconnect", ScanOnDisconnect),
        ESPRESSIO_PROPERTY("tryNextOnFailure", TryNextOnFailure)
    )
};

/// <summary>Persistable station/client configuration including direct credentials and preferred-network profiles.</summary>
struct ClientConfiguration final : Serializable::Serializable<ClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(2)
    ~ClientConfiguration() = default;

    /// <summary>Whether client/station functionality is enabled.</summary>
    bool Enabled = false;
    /// <summary>Direct client SSID retained in externally preferred storage.</summary>
    WiFiString SSID;
    /// <summary>Direct client password retained in externally preferred storage; serialized as a sensitive property.</summary>
    WiFiString Password;
    /// <summary>Address assignment mode for the direct client configuration.</summary>
    AddressMode Addressing = AddressMode::DHCP;
    /// <summary>Static network values used when static addressing is selected.</summary>
    NetworkAddress StaticNetwork{};
    /// <summary>Known network profiles available to the automatic selector, with externally preferred backing storage.</summary>
    WiFiVector<ClientNetworkProfile> Networks;
    /// <summary>Automatic known-network selection behavior.</summary>
    ClientNetworkSelectionConfiguration Selection{};

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY_SENSITIVE("password", Password),
        ESPRESSIO_PROPERTY("addressing", Addressing),
        ESPRESSIO_PROPERTY("staticNetwork", StaticNetwork),
        ESPRESSIO_PROPERTY("networks", Networks),
        ESPRESSIO_PROPERTY("selection", Selection)
    )

    /// <summary>Accepts the supported v1-to-v2 client configuration schema migration.</summary>
    static bool Migrate(Serializable::SerializationNode&, uint32_t fromVersion, uint32_t toVersion) {
        return fromVersion == 1 && toVersion == 2;
    }
};

/// <summary>DHCP server configuration used by the local access point.</summary>
struct DHCPServerConfiguration final : Serializable::Serializable<DHCPServerConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(DHCPServerConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~DHCPServerConfiguration() = default;

    /// <summary>Whether the access-point DHCP server is enabled.</summary>
    bool Enabled = true;
    /// <summary>First IPv4 address in the DHCP lease pool.</summary>
    IPv4Address LeaseStart{192,168,4,2};
    /// <summary>Last IPv4 address in the DHCP lease pool.</summary>
    IPv4Address LeaseEnd{192,168,4,100};
    /// <summary>Lease lifetime in seconds.</summary>
    uint32_t LeaseDurationSeconds = 7200;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("leaseStart", LeaseStart),
        ESPRESSIO_PROPERTY("leaseEnd", LeaseEnd),
        ESPRESSIO_PROPERTY("leaseDurationSeconds", LeaseDurationSeconds)
    )
};

/// <summary>Persistable local access-point configuration.</summary>
struct AccessPointConfiguration final : Serializable::Serializable<AccessPointConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(AccessPointConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    /// <summary>Whether access-point functionality is enabled.</summary>
    bool Enabled = true;
    /// <summary>SSID advertised by the access point and retained in externally preferred storage.</summary>
    WiFiString SSID;
    /// <summary>Access-point password retained in externally preferred storage; serialized as a sensitive property.</summary>
    WiFiString Password;
    /// <summary>Wi-Fi channel used by the access point.</summary>
    uint8_t Channel = 1;
    /// <summary>Whether the SSID is hidden.</summary>
    bool Hidden = false;
    /// <summary>Maximum simultaneous client stations.</summary>
    uint8_t MaximumClients = 4;
    /// <summary>Network addressing exposed by the access point.</summary>
    NetworkAddress Network{};
    /// <summary>DHCP server settings for associated stations.</summary>
    DHCPServerConfiguration DHCP{};

    /// <summary>Creates an access-point configuration with the default 192.168.4.0/24 network.</summary>
    AccessPointConfiguration() {
        Network.Address = IPv4Address(192,168,4,1);
        Network.Gateway = IPv4Address(192,168,4,1);
        Network.SubnetMask = IPv4Address(255,255,255,0);
    }
    ~AccessPointConfiguration() = default;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY_SENSITIVE("password", Password),
        ESPRESSIO_PROPERTY("channel", Channel),
        ESPRESSIO_PROPERTY("hidden", Hidden),
        ESPRESSIO_PROPERTY("maximumClients", MaximumClients),
        ESPRESSIO_PROPERTY("network", Network),
        ESPRESSIO_PROPERTY("dhcp", DHCP)
    )
};

/// <summary>Controls client reconnection timing and retry behavior.</summary>
struct ReconnectPolicy final : Serializable::Serializable<ReconnectPolicy> {
    ESPRESSIO_SERIALIZABLE_TYPE(ReconnectPolicy)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ReconnectPolicy() = default;

    /// <summary>Whether automatic reconnection is enabled.</summary>
    bool Enabled = true;
    /// <summary>Delay before the first reconnection attempt.</summary>
    uint32_t InitialDelayMilliseconds = 1000;
    /// <summary>Maximum delay between reconnection attempts.</summary>
    uint32_t MaximumDelayMilliseconds = 30000;
    /// <summary>Multiplier applied to successive reconnection delays.</summary>
    float BackoffMultiplier = 2.0f;
    /// <summary>Maximum attempts, or zero for no explicit limit.</summary>
    uint32_t MaximumAttempts = 0;
    /// <summary>Maximum time allowed for each connection attempt.</summary>
    uint32_t ConnectionTimeoutMilliseconds = 15000;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("initialDelayMilliseconds", InitialDelayMilliseconds),
        ESPRESSIO_PROPERTY("maximumDelayMilliseconds", MaximumDelayMilliseconds),
        ESPRESSIO_PROPERTY("backoffMultiplier", BackoffMultiplier),
        ESPRESSIO_PROPERTY("maximumAttempts", MaximumAttempts),
        ESPRESSIO_PROPERTY("connectionTimeoutMilliseconds", ConnectionTimeoutMilliseconds)
    )
};

/// <summary>Controls fallback timing for AP-until-client operating mode.</summary>
struct APUntilClientConfiguration final : Serializable::Serializable<APUntilClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(APUntilClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~APUntilClientConfiguration() = default;

    /// <summary>Time before access-point fallback is activated while no client network is usable.</summary>
    uint32_t FallbackTimeoutMilliseconds = 30000;
    /// <summary>Interval between scans while operating in access-point fallback.</summary>
    uint32_t RetryScanIntervalMilliseconds = 30000;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("fallbackTimeoutMilliseconds", FallbackTimeoutMilliseconds),
        ESPRESSIO_PROPERTY("retryScanIntervalMilliseconds", RetryScanIntervalMilliseconds)
    )
};

/// <summary>Top-level persistable configuration for ESPressio Wi-Fi operation.</summary>
struct WiFiConfiguration final : Serializable::Serializable<WiFiConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(3)
    ~WiFiConfiguration() = default;

    /// <summary>Requested Wi-Fi operating mode, including explicit Off mode where supported.</summary>
    WiFiMode Mode = WiFiMode::AccessPoint;
    /// <summary>Hostname advertised/used by the Wi-Fi stack and retained in externally preferred storage.</summary>
    WiFiString Hostname = "espressio";
    /// <summary>Client/station configuration.</summary>
    ClientConfiguration Client{};
    /// <summary>Access-point configuration.</summary>
    AccessPointConfiguration AccessPoint{};
    /// <summary>Client reconnection policy.</summary>
    ReconnectPolicy Reconnect{};
    /// <summary>AP-until-client fallback policy.</summary>
    APUntilClientConfiguration APUntilClient{};
    /// <summary>Requested transmit power in dBm.</summary>
    int8_t TxPowerDbm = 20;
    /// <summary>Whether Wi-Fi power-saving behavior is enabled.</summary>
    bool PowerSave = false;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("mode", Mode),
        ESPRESSIO_PROPERTY("hostname", Hostname),
        ESPRESSIO_PROPERTY("client", Client),
        ESPRESSIO_PROPERTY("accessPoint", AccessPoint),
        ESPRESSIO_PROPERTY("reconnect", Reconnect),
        ESPRESSIO_PROPERTY("apUntilClient", APUntilClient),
        ESPRESSIO_PROPERTY("txPowerDbm", TxPowerDbm),
        ESPRESSIO_PROPERTY("powerSave", PowerSave)
    )

    /// <summary>Accepts supported historical Wi-Fi configuration schema migrations through version 3.</summary>
    static bool Migrate(Serializable::SerializationNode&, uint32_t fromVersion, uint32_t toVersion) {
        return (fromVersion == 1 && toVersion == 2) ||
               (fromVersion == 2 && toVersion == 3) ||
               (fromVersion == 1 && toVersion == 3);
    }
};

} // namespace ESPressio::WiFi
