#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ESPressio::WiFi {

enum class WiFiMode : uint8_t { Disabled, Client, AccessPoint, AccessPointClient };
enum class ClientState : uint8_t { Disabled, Idle, Connecting, Connected, Reconnecting, Failed };
enum class AccessPointState : uint8_t { Disabled, Starting, Active, Failed };
enum class ScanState : uint8_t { Idle, Scanning, Complete, Failed };
enum class NetworkSecurity : uint8_t { Open, WEP, WPA, WPA2, WPA_WPA2, WPA3, WPA2_WPA3, Unknown };
enum class AddressMode : uint8_t { DHCP, Static };

struct IPv4Address {
    std::array<uint8_t, 4> Octets{};

    constexpr bool operator==(const IPv4Address& other) const noexcept {
        return Octets == other.Octets;
    }
    constexpr bool operator!=(const IPv4Address& other) const noexcept { return !(*this == other); }

    std::string ToString() const {
        return std::to_string(Octets[0]) + "." + std::to_string(Octets[1]) + "." +
            std::to_string(Octets[2]) + "." + std::to_string(Octets[3]);
    }
};

struct MacAddress {
    std::array<uint8_t, 6> Octets{};
    constexpr bool operator==(const MacAddress& other) const noexcept { return Octets == other.Octets; }
    constexpr bool operator!=(const MacAddress& other) const noexcept { return !(*this == other); }
};

struct NetworkAddress {
    IPv4Address Address{};
    IPv4Address Gateway{};
    IPv4Address SubnetMask{{255, 255, 255, 0}};
    IPv4Address PrimaryDNS{};
    IPv4Address SecondaryDNS{};
};

struct ScanResult {
    std::string SSID;
    MacAddress BSSID{};
    int32_t RSSI = 0;
    uint8_t Channel = 0;
    NetworkSecurity Security = NetworkSecurity::Unknown;
    bool Hidden = false;
};

struct ClientRuntimeState {
    ClientState State = ClientState::Disabled;
    std::string SSID;
    MacAddress BSSID{};
    int32_t RSSI = 0;
    uint8_t Channel = 0;
    NetworkAddress Network{};
    uint32_t ReconnectAttempt = 0;
};

struct AccessPointRuntimeState {
    AccessPointState State = AccessPointState::Disabled;
    std::string SSID;
    uint8_t Channel = 0;
    NetworkAddress Network{};
    uint16_t ConnectedStations = 0;
};

struct WiFiRuntimeState {
    WiFiMode Mode = WiFiMode::Disabled;
    ClientRuntimeState Client{};
    AccessPointRuntimeState AccessPoint{};
    ScanState Scan = ScanState::Idle;
    uint64_t Revision = 0;
};

} // namespace ESPressio::WiFi
