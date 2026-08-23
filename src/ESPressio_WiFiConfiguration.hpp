#pragma once

#include <cstdint>
#include <string>

#include <ESPressio_Serializable.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

struct ClientConfiguration final : Serializable::Serializable<ClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ClientConfiguration() = default;

    bool Enabled = false;
    std::string SSID;
    std::string Password;
    AddressMode Addressing = AddressMode::DHCP;
    NetworkAddress StaticNetwork{};

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY_SENSITIVE("password", Password),
        ESPRESSIO_PROPERTY("addressing", Addressing),
        ESPRESSIO_PROPERTY("staticNetwork", StaticNetwork)
    )
};

struct DHCPServerConfiguration final : Serializable::Serializable<DHCPServerConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(DHCPServerConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~DHCPServerConfiguration() = default;

    bool Enabled = true;
    IPv4Address LeaseStart{192, 168, 4, 2};
    IPv4Address LeaseEnd{192, 168, 4, 100};
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
    ~AccessPointConfiguration() = default;

    bool Enabled = true;
    std::string SSID;
    std::string Password;
    uint8_t Channel = 1;
    bool Hidden = false;
    uint8_t MaximumClients = 4;
    NetworkAddress Network{};
    DHCPServerConfiguration DHCP{};

    AccessPointConfiguration() {
        Network.Address = IPv4Address(192, 168, 4, 1);
        Network.Gateway = IPv4Address(192, 168, 4, 1);
        Network.SubnetMask = IPv4Address(255, 255, 255, 0);
    }

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
    uint32_t MaximumAttempts = 0;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("initialDelayMilliseconds", InitialDelayMilliseconds),
        ESPRESSIO_PROPERTY("maximumDelayMilliseconds", MaximumDelayMilliseconds),
        ESPRESSIO_PROPERTY("maximumAttempts", MaximumAttempts)
    )
};

struct WiFiConfiguration final : Serializable::Serializable<WiFiConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~WiFiConfiguration() = default;

    WiFiMode Mode = WiFiMode::AccessPoint;
    std::string Hostname = "espressio";
    ClientConfiguration Client{};
    AccessPointConfiguration AccessPoint{};
    ReconnectPolicy Reconnect{};
    int8_t TxPowerDbm = 20;
    bool PowerSave = false;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("mode", Mode),
        ESPRESSIO_PROPERTY("hostname", Hostname),
        ESPRESSIO_PROPERTY("client", Client),
        ESPRESSIO_PROPERTY("accessPoint", AccessPoint),
        ESPRESSIO_PROPERTY("reconnect", Reconnect),
        ESPRESSIO_PROPERTY("txPowerDbm", TxPowerDbm),
        ESPRESSIO_PROPERTY("powerSave", PowerSave)
    )
};

} // namespace ESPressio::WiFi
