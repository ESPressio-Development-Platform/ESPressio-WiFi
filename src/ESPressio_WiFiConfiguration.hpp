#pragma once

#include <cstdint>
#include <string>

#include <ESPressio_Serializable.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::WiFi {

struct ClientConfiguration final : Serializable::Serializable<ClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(ClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    bool Enabled = false;
    std::string SSID;
    std::string Password;
    AddressMode Addressing = AddressMode::DHCP;
    NetworkAddress StaticNetwork{};

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY_SENSITIVE("password", Password)
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
    NetworkAddress Network{{{192, 168, 4, 1}}, {{192, 168, 4, 1}}, {{255, 255, 255, 0}}, {}, {}};
    bool DHCPServerEnabled = true;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("enabled", Enabled),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY_SENSITIVE("password", Password),
        ESPRESSIO_PROPERTY("channel", Channel),
        ESPRESSIO_PROPERTY("hidden", Hidden),
        ESPRESSIO_PROPERTY("maximumClients", MaximumClients),
        ESPRESSIO_PROPERTY("dhcpServerEnabled", DHCPServerEnabled)
    )
};

struct ReconnectPolicy final : Serializable::Serializable<ReconnectPolicy> {
    ESPRESSIO_SERIALIZABLE_TYPE(ReconnectPolicy)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)

    bool Enabled = true;
    uint32_t InitialDelayMilliseconds = 1000;
    uint32_t MaximumDelayMilliseconds = 30000;
    uint32_t MaximumAttempts = 0; // zero = unlimited

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

    WiFiMode Mode = WiFiMode::AccessPoint;
    std::string Hostname = "espressio";
    ClientConfiguration Client{};
    AccessPointConfiguration AccessPoint{};
    ReconnectPolicy Reconnect{};
    int8_t TxPowerDbm = 20;
    bool PowerSave = false;

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("hostname", Hostname),
        ESPRESSIO_PROPERTY("client", Client),
        ESPRESSIO_PROPERTY("accessPoint", AccessPoint),
        ESPRESSIO_PROPERTY("reconnect", Reconnect),
        ESPRESSIO_PROPERTY("txPowerDbm", TxPowerDbm),
        ESPRESSIO_PROPERTY("powerSave", PowerSave)
    )
};

} // namespace ESPressio::WiFi
