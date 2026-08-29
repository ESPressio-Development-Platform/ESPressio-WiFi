#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <ESPressio_Memory.hpp>
#include <ESPressio_Serializable.hpp>

namespace ESPressio::WiFi {

/// <summary>Memory policy used for Wi-Fi dynamic state that does not require internal or DMA-capable RAM.</summary>
inline constexpr auto WiFiMemoryPolicy =
    System::Memory::MemoryPolicy::ExternalPreferred;

/// <summary>Wi-Fi text whose dynamic storage prefers external memory.</summary>
using WiFiString = System::Memory::String<WiFiMemoryPolicy>;

/// <summary>Wi-Fi dynamic collection whose backing storage prefers external memory.</summary>
template<typename T>
using WiFiVector = System::Memory::Vector<T, WiFiMemoryPolicy>;

/// <summary>Application-level Wi-Fi operating mode.</summary>
/// <remarks>APUntilClient prefers STA connectivity and exposes an AP only as fallback. Off is the canonical explicit radio-off mode; Disabled is retained for compatibility and has the same platform effect.</remarks>
enum class WiFiMode : uint8_t { Disabled, Client, AccessPoint, AccessPointClient, APUntilClient, Off };
/// <summary>Runtime state of the client/station interface.</summary>
enum class ClientState : uint8_t { Disabled, Idle, Connecting, Connected, Reconnecting, Disconnecting, Disconnected, Failed };
/// <summary>Runtime state of the local access point.</summary>
enum class AccessPointState : uint8_t { Disabled, Starting, Active, Failed };
/// <summary>Runtime state of Wi-Fi scanning.</summary>
enum class ScanState : uint8_t { Idle, Scanning, Complete, Failed };
/// <summary>Security classification reported for a scanned network.</summary>
enum class NetworkSecurity : uint8_t { Open, WEP, WPA, WPA2, WPA_WPA2, WPA3, WPA2_WPA3, Unknown };
/// <summary>Selects dynamic DHCP or configured static addressing.</summary>
enum class AddressMode : uint8_t { DHCP, Static };
/// <summary>Runtime state of automatic preferred-client-network selection.</summary>
enum class ClientNetworkSelectionState : uint8_t { Idle, Scanning, Selecting, Connecting, Connected, NoKnownNetworkAvailable, Exhausted };
/// <summary>Runtime state of AP-until-client fallback behavior.</summary>
enum class APUntilClientState : uint8_t { Inactive, SeekingClient, FallbackAccessPoint, ClientConnected };

/// <summary>Authoritative physical mode of the shared Wi-Fi radio.</summary>
enum class WiFiRadioMode : uint8_t {
    Off = 0,
    Station,
    AccessPoint,
    AccessPointStation
};

/// <summary>Reason associated with a low-level shared-radio state transition.</summary>
enum class WiFiRadioTransitionReason : uint8_t {
    Configuration = 0,
    ClientConnect,
    ClientDisconnect,
    AccessPointStart,
    AccessPointStop,
    Scan,
    DriverStateChange
};

/// <summary>Serializable IPv4 address value.</summary>
struct IPv4Address final : Serializable::Serializable<IPv4Address> {
    ESPRESSIO_SERIALIZABLE_TYPE(IPv4Address)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    /// <summary>Address octets in network display order.</summary>
    std::array<uint8_t,4> Octets{};
    IPv4Address() = default;
    /// <summary>Constructs an IPv4 address from its four octets.</summary>
    IPv4Address(uint8_t a,uint8_t b,uint8_t c,uint8_t d) : Octets{{a,b,c,d}} {}
    ~IPv4Address() = default;
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("octets", Octets))
    bool operator==(const IPv4Address& other) const noexcept { return Octets==other.Octets; }
    bool operator!=(const IPv4Address& other) const noexcept { return !(*this==other); }
    /// <summary>Returns dotted-decimal text in externally preferred storage.</summary>
    WiFiString ToString() const {
        char buffer[16]{};
        const int written = std::snprintf(
            buffer,
            sizeof(buffer),
            "%u.%u.%u.%u",
            static_cast<unsigned>(Octets[0]),
            static_cast<unsigned>(Octets[1]),
            static_cast<unsigned>(Octets[2]),
            static_cast<unsigned>(Octets[3])
        );
        return written > 0
            ? WiFiString(buffer, static_cast<std::size_t>(written))
            : WiFiString{};
    }
};

/// <summary>Serializable six-octet MAC address value.</summary>
struct MacAddress final : Serializable::Serializable<MacAddress> {
    ESPRESSIO_SERIALIZABLE_TYPE(MacAddress)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~MacAddress() = default;
    std::array<uint8_t,6> Octets{};
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("octets", Octets))
    bool operator==(const MacAddress& other) const noexcept { return Octets==other.Octets; }
    bool operator!=(const MacAddress& other) const noexcept { return !(*this==other); }
};

/// <summary>Authoritative physical state of the shared 2.4 GHz Wi-Fi radio.</summary>
/// <remarks>This intentionally differs from WiFiRuntimeState: infrastructure consumers such as ESP-NOW require native interface, channel, scan, and MAC facts rather than only application semantics.</remarks>
struct WiFiRadioState {
    WiFiRadioMode Mode = WiFiRadioMode::Off;
    bool StationInterfaceActive = false;
    bool StationConnected = false;
    bool AccessPointInterfaceActive = false;
    bool Scanning = false;
    uint8_t Channel = 0;
    MacAddress StationMAC{};
    MacAddress AccessPointMAC{};

    bool operator==(const WiFiRadioState& other) const noexcept {
        return Mode == other.Mode &&
            StationInterfaceActive == other.StationInterfaceActive &&
            StationConnected == other.StationConnected &&
            AccessPointInterfaceActive == other.AccessPointInterfaceActive &&
            Scanning == other.Scanning &&
            Channel == other.Channel &&
            StationMAC == other.StationMAC &&
            AccessPointMAC == other.AccessPointMAC;
    }
    bool operator!=(const WiFiRadioState& other) const noexcept { return !(*this == other); }
};

/// <summary>Serializable IP addressing configuration for an interface.</summary>
struct NetworkAddress final : Serializable::Serializable<NetworkAddress> {
    ESPRESSIO_SERIALIZABLE_TYPE(NetworkAddress)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~NetworkAddress() = default;
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

/// <summary>Serializable description of one network returned by Wi-Fi scanning.</summary>
struct ScanResult final : Serializable::Serializable<ScanResult> {
    ESPRESSIO_SERIALIZABLE_TYPE(ScanResult)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
    ~ScanResult() = default;
    WiFiString SSID;
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

/// <summary>Resolved known-network candidate considered by automatic client selection.</summary>
struct ClientNetworkCandidate {
    WiFiString SSID;
    MacAddress BSSID{};
    uint16_t Priority = 0;
    int32_t RSSI = 0;
    uint8_t Channel = 0;
    std::size_t ProfileIndex = 0;
};

/// <summary>Current state and selected candidate metadata for automatic client-network selection.</summary>
struct ClientNetworkSelectionRuntimeState {
    ClientNetworkSelectionState State = ClientNetworkSelectionState::Idle;
    WiFiString SelectedSSID;
    uint16_t SelectedPriority = 0;
    std::size_t SelectedProfileIndex = 0;
    std::size_t EligibleCandidateCount = 0;
};

/// <summary>Current client/station runtime state.</summary>
struct ClientRuntimeState {
    ClientState State=ClientState::Disabled;
    WiFiString SSID;
    MacAddress BSSID{};
    int32_t RSSI=0;
    uint8_t Channel=0;
    NetworkAddress Network{};
    uint32_t ReconnectAttempt=0;
    ClientNetworkSelectionRuntimeState Selection{};
};

/// <summary>Current access-point runtime state.</summary>
struct AccessPointRuntimeState {
    AccessPointState State=AccessPointState::Disabled;
    WiFiString SSID;
    uint8_t Channel=0;
    NetworkAddress Network{};
    uint16_t ConnectedStations=0;
};

/// <summary>Current AP-until-client fallback runtime state.</summary>
struct APUntilClientRuntimeState {
    APUntilClientState State = APUntilClientState::Inactive;
    bool FallbackAccessPointActive = false;
    uint64_t FallbackDeadlineMilliseconds = 0;
    uint64_t NextRetryMilliseconds = 0;
};

/// <summary>Aggregate application-level Wi-Fi runtime state and revision.</summary>
struct WiFiRuntimeState {
    WiFiMode Mode=WiFiMode::Off;
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
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::APUntilClient, "ap-until-client"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::WiFiMode::Off, "off")
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
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::APUntilClientState::ClientConnected, "client-connected"),
    ESPRESSIO_ENUM_VALUE(ESPressio::WiFi::APUntilClientState::ClientConnected, "client-connected")
)
