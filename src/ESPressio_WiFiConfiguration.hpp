#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <ESPressio_Serializable.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

struct ClientNetworkProfile final : Serializable::Serializable<ClientNetworkProfile> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientNetworkProfile)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ClientNetworkProfile() = default;

    std::string SSID;
    std::string Password;
    uint16_t Priority = 100;
    bool Enabled = true;
    AddressMode Addressing = AddressMode::DHCP;
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

struct ClientNetworkSelectionConfiguration final
    : Serializable::Serializable<ClientNetworkSelectionConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientNetworkSelectionConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ClientNetworkSelectionConfiguration() = default;

    bool AutomaticSelection = true;
    bool ScanOnStartup = true;
    bool ScanOnDisconnect = true;
    bool TryNextOnFailure = true;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("automaticSelection", AutomaticSelection),
        ESPRESSIO_PROPERTY("scanOnStartup", ScanOnStartup),
        ESPRESSIO_PROPERTY("scanOnDisconnect", ScanOnDisconnect),
        ESPRESSIO_PROPERTY("tryNextOnFailure", TryNextOnFailure)
    )
};

struct ClientConfiguration final : Serializable::Serializable<ClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(2)
    ~ClientConfiguration() = default;

    bool Enabled = false;
    std::string SSID;
    std::string Password;
    AddressMode Addressing = AddressMode::DHCP;
    NetworkAddress StaticNetwork{};
    std::vector<ClientNetworkProfile> Networks;
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

    static bool Migrate(Serializable::SerializationNode&, uint32_t fromVersion, uint32_t toVersion) {
        return fromVersion == 1 && toVersion == 2;
    }
};

struct DHCPServerConfiguration final : Serializable::Serializable<DHCPServerConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(DHCPServerConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~DHCPServerConfiguration() = default;

    bool Enabled = true;
    IPv4Address LeaseStart{192,168,4,2};
    IPv4Address LeaseEnd{192,168,4,100};
    uint32_t LeaseDurationSeconds = 7200;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("leaseStart", LeaseStart),
        ESPRESSIO_PROPERTY("leaseEnd", LeaseEnd),
        ESPRESSIO_PROPERTY("leaseDurationSeconds", LeaseDurationSeconds)
    )
};

struct AccessPointConfiguration final : Serializable::Serializable<AccessPointConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(AccessPointConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    bool Enabled = true;
    std::string SSID;
    std::string Password;
    uint8_t Channel = 1;
    bool Hidden = false;
    uint8_t MaximumClients = 4;
    NetworkAddress Network{};
    DHCPServerConfiguration DHCP{};

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

struct ReconnectPolicy final : Serializable::Serializable<ReconnectPolicy> {
    ESPRESSIO_SERIALIZABLE_TYPE(ReconnectPolicy)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ReconnectPolicy() = default;

    bool Enabled = true;
    uint32_t InitialDelayMilliseconds = 1000;
    uint32_t MaximumDelayMilliseconds = 30000;
    float BackoffMultiplier = 2.0f;
    uint32_t MaximumAttempts = 0;
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

struct APUntilClientConfiguration final : Serializable::Serializable<APUntilClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(APUntilClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~APUntilClientConfiguration() = default;

    uint32_t FallbackTimeoutMilliseconds = 30000;
    uint32_t RetryScanIntervalMilliseconds = 30000;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("fallbackTimeoutMilliseconds", FallbackTimeoutMilliseconds),
        ESPRESSIO_PROPERTY("retryScanIntervalMilliseconds", RetryScanIntervalMilliseconds)
    )
};

struct WiFiConfiguration final : Serializable::Serializable<WiFiConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(3)
    ~WiFiConfiguration() = default;

    WiFiMode Mode = WiFiMode::AccessPoint;
    std::string Hostname = "espressio";
    ClientConfiguration Client{};
    AccessPointConfiguration AccessPoint{};
    ReconnectPolicy Reconnect{};
    APUntilClientConfiguration APUntilClient{};
    int8_t TxPowerDbm = 20;
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

    static bool Migrate(Serializable::SerializationNode&, uint32_t fromVersion, uint32_t toVersion) {
        return (fromVersion == 1 && toVersion == 2) ||
               (fromVersion == 2 && toVersion == 3) ||
               (fromVersion == 1 && toVersion == 3);
    }
};

} // namespace ESPressio::WiFi
