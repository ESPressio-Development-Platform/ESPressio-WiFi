#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <ESPressio_Serializable.hpp>

namespace ESPressio::WiFi {

// APUntilClient is a conditional fallback mode: STA is preferred, while the AP
// is exposed only until Client connectivity succeeds.
enum class WiFiMode : uint8_t { Disabled, Client, AccessPoint, AccessPointClient, APUntilClient };
enum class ClientState : uint8_t { Disabled, Idle, Connecting, Connected, Reconnecting, Disconnecting, Disconnected, Failed };
enum class AccessPointState : uint8_t { Disabled, Starting, Active, Failed };
enum class ScanState : uint8_t { Idle, Scanning, Complete, Failed };
enum class NetworkSecurity : uint8_t { Open, WEP, WPA, WPA2, WPA_WPA2, WPA3, WPA2_WPA3, Unknown };
enum class AddressMode : uint8_t { DHCP, Static };
enum class ClientNetworkSelectionState : uint8_t { Idle, Scanning, Selecting, Connecting, Connected, NoKnownNetworkAvailable, Exhausted };
enum class APUntilClientState : uint8_t { Inactive, SeekingClient, FallbackAccessPoint, ClientConnected };

struct IPv4Address final : Serializable::Serializable<IPv4Address> {
    ESPRESSIO_SERIALIZABLE_TYPE(IPv4Address)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    std::array<uint8_t,4> Octets{};
    IPv4Address() = default;
    IPv4Address(uint8_t a,uint8_t b,uint8_t c,uint8_t d) : Octets{{a,b,c,d}} {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("octets", Octets))
    bool operator==(const IPv4Address& other) const noexcept { return Octets==other.Octets; }
    bool operator!=(const IPv4Address& other) const noexcept { return !(*this==other); }
    std::string ToString() const { return std::to_string(Octets[0])+"."+std::to_string(Octets[1])+"."+std::to_string(Octets[2])+"."+std::to_string(Octets[3]); }
};

struct MacAddress final : Serializable::Serializable<MacAddress> {
    ESPRESSIO_SERIALIZABLE_TYPE(MacAddress)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    std::array<uint8_t,6> Octets{};
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("octets", Octets))
    bool operator==(const MacAddress& other) const noexcept { return Octets==other.Octets; }
    bool operator!=(const MacAddress& other) const noexcept { return !(*this==other); }
};

struct NetworkAddress final : Serializable::Serializable<NetworkAddress> {
    ESPRESSIO_SERIALIZABLE_TYPE(NetworkAddress)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    IPv4Address Address{};
    IPv4Address Gateway{};
    IPv4Address SubnetMask{255,255,255,0};
    IPv4Address PrimaryDNS{};
    IPv4Address SecondaryDNS{};
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("address", Address),
        ESPRESSIO_PROPERTY("gateway", Gateway),
        ESPRESSIO_PROPERTY("subnetMask", SubnetMask),
        ESPRESSIO_PROPERTY("primaryDNS", PrimaryDNS),
        ESPRESSIO_PROPERTY("secondaryDNS", SecondaryDNS)
    )
};

struct ScanResult final : Serializable::Serializable<ScanResult> {
    ESPRESSIO_SERIALIZABLE_TYPE(ScanResult)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    std::string SSID;
    MacAddress BSSID{};
    int32_t RSSI=0;
    uint8_t Channel=0;
    NetworkSecurity Security=NetworkSecurity::Unknown;
    bool Hidden=false;
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY("bssid", BSSID),
        ESPRESSIO_PROPERTY("rssi", RSSI),
        ESPRESSIO_PROPERTY("channel", Channel),
        ESPRESSIO_PROPERTY("security", Security),
        ESPRESSIO_PROPERTY("hidden", Hidden)
    )
};

struct ClientNetworkCandidate {
    std::string SSID;
    MacAddress BSSID{};
    uint16_t Priority = 0;
    int32_t RSSI = 0;
    uint8_t Channel = 0;
    std::size_t ProfileIndex = 0;
};

struct ClientNetworkSelectionRuntimeState {
    ClientNetworkSelectionState State = ClientNetworkSelectionState::Idle;
    std::string SelectedSSID;
    uint16_t SelectedPriority = 0;
    std::size_t SelectedProfileIndex = 0;
    std::size_t EligibleCandidateCount = 0;
};

struct ClientRuntimeState {
    ClientState State=ClientState::Disabled;
    std::string SSID;
    MacAddress BSSID{};
    int32_t RSSI=0;
    uint8_t Channel=0;
    NetworkAddress Network{};
    uint32_t ReconnectAttempt=0;
    ClientNetworkSelectionRuntimeState Selection{};
};

struct AccessPointRuntimeState {
    AccessPointState State=AccessPointState::Disabled;
    std::string SSID;
    uint8_t Channel=0;
    NetworkAddress Network{};
    uint16_t ConnectedStations=0;
};

struct APUntilClientRuntimeState {
    APUntilClientState State = APUntilClientState::Inactive;
    bool FallbackAccessPointActive = false;
    uint64_t FallbackDeadlineMilliseconds = 0;
    uint64_t NextRetryMilliseconds = 0;
};

struct WiFiRuntimeState {
    WiFiMode Mode=WiFiMode::Disabled;
    ClientRuntimeState Client{};
    AccessPointRuntimeState AccessPoint{};
    APUntilClientRuntimeState APUntilClient{};
    ScanState Scan=ScanState::Idle;
    uint64_t Revision=0;
};

} // namespace ESPressio::WiFi

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::WiFiMode,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::Disabled, "disabled"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::Client, "client"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::AccessPoint, "access-point"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::AccessPointClient, "access-point-client"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::APUntilClient, "ap-until-client")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::ClientState,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Disabled, "disabled"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Idle, "idle"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Connecting, "connecting"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Connected, "connected"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Reconnecting, "reconnecting"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Disconnecting, "disconnecting"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Disconnected, "disconnected"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientState::Failed, "failed")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::AccessPointState,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::AccessPointState::Disabled, "disabled"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::AccessPointState::Starting, "starting"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::AccessPointState::Active, "active"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::AccessPointState::Failed, "failed")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::ScanState,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ScanState::Idle, "idle"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ScanState::Scanning, "scanning"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ScanState::Complete, "complete"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ScanState::Failed, "failed")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::NetworkSecurity,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::Open, "open"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::WEP, "wep"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::WPA, "wpa"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::WPA2, "wpa2"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::WPA_WPA2, "wpa-wpa2"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::WPA3, "wpa3"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::WPA2_WPA3, "wpa2-wpa3"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::NetworkSecurity::Unknown, "unknown")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::AddressMode,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::AddressMode::DHCP, "dhcp"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::AddressMode::Static, "static")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::ClientNetworkSelectionState,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::Idle, "idle"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::Scanning, "scanning"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::Selecting, "selecting"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::Connecting, "connecting"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::Connected, "connected"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::NoKnownNetworkAvailable, "no-known-network-available"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::ClientNetworkSelectionState::Exhausted, "exhausted")
)

ESPRESSIO_ENUM_MAPPING(
    ESPressio::WiFi::APUntilClientState,
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::APUntilClientState::Inactive, "inactive"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::APUntilClientState::SeekingClient, "seeking-client"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::APUntilClientState::FallbackAccessPoint, "fallback-access-point"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::APUntilClientState::ClientConnected, "client-connected")
)
