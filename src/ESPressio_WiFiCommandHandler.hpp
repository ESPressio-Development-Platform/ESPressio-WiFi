#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

#include <ESPressio_SerializableCommand.hpp>
#include <ESPressio_CommandRuntime.hpp>
#include <ESPressio_TypeDirectory.hpp>

#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

/// Finite bounds for the dynamic/admin Command projection. WiFiManager retains
/// its existing WiFi-owned storage semantics; these limits bound Primitive input
/// before family decode/construction.
inline constexpr std::size_t WiFiAdministrativeSSIDMaximumBytes = 32;
inline constexpr std::size_t WiFiAdministrativePasswordMaximumBytes = 64;
inline constexpr std::size_t WiFiAdministrativeHostnameMaximumBytes = 64;
inline constexpr std::size_t WiFiAdministrativeMaximumNetworks = 8;

struct WiFiAdministrativeClientNetworkProfile final
    : Serializable::Serializable<WiFiAdministrativeClientNetworkProfile> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAdministrativeClientNetworkProfile)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> SSID;
    Serializable::BoundedString<WiFiAdministrativePasswordMaximumBytes> Password;
    std::uint16_t Priority = 100;
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

struct WiFiAdministrativeClientConfiguration final
    : Serializable::Serializable<WiFiAdministrativeClientConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAdministrativeClientConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    bool Enabled = false;
    Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> SSID;
    Serializable::BoundedString<WiFiAdministrativePasswordMaximumBytes> Password;
    AddressMode Addressing = AddressMode::DHCP;
    NetworkAddress StaticNetwork{};
    Serializable::BoundedVector<
        WiFiAdministrativeClientNetworkProfile,
        WiFiAdministrativeMaximumNetworks
    > Networks;
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
};

struct WiFiAdministrativeAccessPointConfiguration final
    : Serializable::Serializable<WiFiAdministrativeAccessPointConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAdministrativeAccessPointConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    bool Enabled = true;
    Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> SSID;
    Serializable::BoundedString<WiFiAdministrativePasswordMaximumBytes> Password;
    std::uint8_t Channel = 1;
    bool Hidden = false;
    std::uint8_t MaximumClients = 4;
    NetworkAddress Network{};
    DHCPServerConfiguration DHCP{};

    WiFiAdministrativeAccessPointConfiguration() noexcept {
        Network.Address = IPv4Address(192,168,4,1);
        Network.Gateway = IPv4Address(192,168,4,1);
        Network.SubnetMask = IPv4Address(255,255,255,0);
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

struct WiFiAdministrativeConfiguration final
    : Serializable::Serializable<WiFiAdministrativeConfiguration> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAdministrativeConfiguration)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFiMode Mode = WiFiMode::AccessPoint;
    Serializable::BoundedString<WiFiAdministrativeHostnameMaximumBytes> Hostname;
    WiFiAdministrativeClientConfiguration Client{};
    WiFiAdministrativeAccessPointConfiguration AccessPoint{};
    ReconnectPolicy Reconnect{};
    APUntilClientConfiguration APUntilClient{};
    std::int8_t TxPowerDbm = 20;
    bool PowerSave = false;

    WiFiAdministrativeConfiguration() noexcept { (void)Hostname.assign("espressio"); }

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
};

enum class WiFiCommandOutcome : std::uint8_t {
    Success,
    InvalidConfiguration,
    NotSupported,
    Busy,
    PlatformError,
    PersistenceNotConfigured,
    PersistenceNotFound,
    PersistenceStorageError,
    PersistenceSerializationError,
    PersistenceProtectionError,
    NetworkNotFound
};

struct WiFiCommandResponse final : Serializable::Serializable<WiFiCommandResponse> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiCommandResponse)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFiCommandOutcome Outcome = WiFiCommandOutcome::Success;
    WiFiCommandResponse() = default;
    explicit WiFiCommandResponse(WiFiCommandOutcome outcome) noexcept : Outcome(outcome) {}
    constexpr bool Succeeded() const noexcept { return Outcome == WiFiCommandOutcome::Success; }
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("outcome", Outcome))
};

namespace WiFiCommandDetail {

template<std::size_t N>
inline WiFiString ToNativeString(const Serializable::BoundedString<N>& source) {
    WiFiString result;
    result.assign(source.data(), source.size());
    return result;
}

inline ClientNetworkProfile ToNative(const WiFiAdministrativeClientNetworkProfile& source) {
    ClientNetworkProfile result;
    result.SSID = ToNativeString(source.SSID);
    result.Password = ToNativeString(source.Password);
    result.Priority = source.Priority;
    result.Enabled = source.Enabled;
    result.Addressing = source.Addressing;
    result.StaticNetwork = source.StaticNetwork;
    return result;
}

inline WiFiConfiguration ToNative(const WiFiAdministrativeConfiguration& source) {
    WiFiConfiguration result;
    result.Mode = source.Mode;
    result.Hostname = ToNativeString(source.Hostname);
    result.Client.Enabled = source.Client.Enabled;
    result.Client.SSID = ToNativeString(source.Client.SSID);
    result.Client.Password = ToNativeString(source.Client.Password);
    result.Client.Addressing = source.Client.Addressing;
    result.Client.StaticNetwork = source.Client.StaticNetwork;
    result.Client.Selection = source.Client.Selection;
    result.Client.Networks.clear();
    result.Client.Networks.reserve(source.Client.Networks.size());
    for (const auto& profile : source.Client.Networks)
        result.Client.Networks.push_back(ToNative(profile));
    result.AccessPoint.Enabled = source.AccessPoint.Enabled;
    result.AccessPoint.SSID = ToNativeString(source.AccessPoint.SSID);
    result.AccessPoint.Password = ToNativeString(source.AccessPoint.Password);
    result.AccessPoint.Channel = source.AccessPoint.Channel;
    result.AccessPoint.Hidden = source.AccessPoint.Hidden;
    result.AccessPoint.MaximumClients = source.AccessPoint.MaximumClients;
    result.AccessPoint.Network = source.AccessPoint.Network;
    result.AccessPoint.DHCP = source.AccessPoint.DHCP;
    result.Reconnect = source.Reconnect;
    result.APUntilClient = source.APUntilClient;
    result.TxPowerDbm = source.TxPowerDbm;
    result.PowerSave = source.PowerSave;
    return result;
}

inline WiFiCommandResponse FromStatus(WiFiStatus status) noexcept {
    switch (status) {
        case WiFiStatus::Success: return WiFiCommandResponse{WiFiCommandOutcome::Success};
        case WiFiStatus::InvalidConfiguration: return WiFiCommandResponse{WiFiCommandOutcome::InvalidConfiguration};
        case WiFiStatus::NotSupported: return WiFiCommandResponse{WiFiCommandOutcome::NotSupported};
        case WiFiStatus::Busy: return WiFiCommandResponse{WiFiCommandOutcome::Busy};
        case WiFiStatus::PlatformError: return WiFiCommandResponse{WiFiCommandOutcome::PlatformError};
    }
    return WiFiCommandResponse{WiFiCommandOutcome::PlatformError};
}

inline WiFiCommandResponse FromStoreStatus(const WiFiConfigurationStoreResult& result) noexcept {
    switch (result.Status) {
        case WiFiConfigurationStoreStatus::Success: return WiFiCommandResponse{WiFiCommandOutcome::Success};
        case WiFiConfigurationStoreStatus::NotConfigured: return WiFiCommandResponse{WiFiCommandOutcome::PersistenceNotConfigured};
        case WiFiConfigurationStoreStatus::NotFound: return WiFiCommandResponse{WiFiCommandOutcome::PersistenceNotFound};
        case WiFiConfigurationStoreStatus::StorageError: return WiFiCommandResponse{WiFiCommandOutcome::PersistenceStorageError};
        case WiFiConfigurationStoreStatus::SerializationError: return WiFiCommandResponse{WiFiCommandOutcome::PersistenceSerializationError};
        case WiFiConfigurationStoreStatus::ProtectionError: return WiFiCommandResponse{WiFiCommandOutcome::PersistenceProtectionError};
    }
    return WiFiCommandResponse{WiFiCommandOutcome::PersistenceStorageError};
}

} // namespace WiFiCommandDetail

// Command TypeIds are explicit stable WiFi-owned assignments, independent of
// CanonicalName. Every operation is response-bearing because the predecessor
// administrative contract returned an operation result; generic dynamic tools
// therefore see RequesterRequired rather than silently discarding status.
#define ESPRESSIO_WIFI_COMMAND_METADATA(IdValue, NameValue) \
    static constexpr Command::CommandTypeId TypeId{IdValue}; \
    static constexpr std::string_view CanonicalName = NameValue; \
    static constexpr std::size_t MaximumLiveInstances = 2; \
    static constexpr std::size_t MaximumPendingExecutions = 1; \
    static constexpr std::size_t MaximumPendingResponses = 2; \
    using ExecutionAdmissionPolicy = Command::RequiredExecution

class WiFiConfigureCommand final
    : public Command::SerializableCommand<WiFiConfigureCommand, WiFiCommandResponse> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiConfigureCommand)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_WIFI_COMMAND_METADATA(0x5749464900020001ULL, "ESPressio.WiFi.Configure");
    WiFiAdministrativeConfiguration Configuration{};
    WiFiConfigureCommand() = default;
    explicit WiFiConfigureCommand(WiFiAdministrativeConfiguration configuration) noexcept
        : Configuration(std::move(configuration)) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("configuration", Configuration))
};

#define ESPRESSIO_WIFI_EMPTY_COMMAND(TypeName, IdValue, Canonical) \
class TypeName final : public Command::SerializableCommand<TypeName, WiFiCommandResponse> { \
    ESPRESSIO_SERIALIZABLE_TYPE(TypeName) \
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1) \
public: \
    ESPRESSIO_WIFI_COMMAND_METADATA(IdValue, Canonical); \
    ESPRESSIO_SERIALIZABLE_PROPERTIES() \
}

ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiDisableCommand, 0x5749464900020002ULL, "ESPressio.WiFi.Disable");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiConnectClientCommand, 0x5749464900020003ULL, "ESPressio.WiFi.Client.Connect");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiDisconnectClientCommand, 0x5749464900020004ULL, "ESPressio.WiFi.Client.Disconnect");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiStartAccessPointCommand, 0x5749464900020005ULL, "ESPressio.WiFi.AccessPoint.Start");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiStopAccessPointCommand, 0x5749464900020006ULL, "ESPressio.WiFi.AccessPoint.Stop");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiScanCommand, 0x5749464900020007ULL, "ESPressio.WiFi.Scan");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiRetryKnownNetworksCommand, 0x5749464900020008ULL, "ESPressio.WiFi.Client.RetryKnownNetworks");
ESPRESSIO_WIFI_EMPTY_COMMAND(WiFiSaveConfigurationCommand, 0x574946490002000CULL, "ESPressio.WiFi.Configuration.Save");

#undef ESPRESSIO_WIFI_EMPTY_COMMAND

class WiFiUpsertClientNetworkCommand final
    : public Command::SerializableCommand<WiFiUpsertClientNetworkCommand, WiFiCommandResponse> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiUpsertClientNetworkCommand)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_WIFI_COMMAND_METADATA(0x5749464900020009ULL, "ESPressio.WiFi.Client.Network.Upsert");
    WiFiAdministrativeClientNetworkProfile Profile{};
    WiFiUpsertClientNetworkCommand() = default;
    explicit WiFiUpsertClientNetworkCommand(WiFiAdministrativeClientNetworkProfile profile) noexcept
        : Profile(std::move(profile)) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("profile", Profile))
};

class WiFiRemoveClientNetworkCommand final
    : public Command::SerializableCommand<WiFiRemoveClientNetworkCommand, WiFiCommandResponse> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiRemoveClientNetworkCommand)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_WIFI_COMMAND_METADATA(0x574946490002000AULL, "ESPressio.WiFi.Client.Network.Remove");
    Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> SSID;
    WiFiRemoveClientNetworkCommand() = default;
    explicit WiFiRemoveClientNetworkCommand(Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> ssid) noexcept
        : SSID(std::move(ssid)) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("ssid", SSID))
};

class WiFiSetClientNetworkPriorityCommand final
    : public Command::SerializableCommand<WiFiSetClientNetworkPriorityCommand, WiFiCommandResponse> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiSetClientNetworkPriorityCommand)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_WIFI_COMMAND_METADATA(0x574946490002000BULL, "ESPressio.WiFi.Client.Network.Priority");
    Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> SSID;
    std::uint16_t Priority = 100;
    WiFiSetClientNetworkPriorityCommand() = default;
    WiFiSetClientNetworkPriorityCommand(
        Serializable::BoundedString<WiFiAdministrativeSSIDMaximumBytes> ssid,
        std::uint16_t priority
    ) noexcept : SSID(std::move(ssid)), Priority(priority) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("priority", Priority)
    )
};

class WiFiLoadConfigurationCommand final
    : public Command::SerializableCommand<WiFiLoadConfigurationCommand, WiFiCommandResponse> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiLoadConfigurationCommand)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_WIFI_COMMAND_METADATA(0x574946490002000DULL, "ESPressio.WiFi.Configuration.Load");
    bool Apply = true;
    WiFiLoadConfigurationCommand() = default;
    explicit WiFiLoadConfigurationCommand(bool apply) noexcept : Apply(apply) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("apply", Apply))
};

#undef ESPRESSIO_WIFI_COMMAND_METADATA

/// Final WiFi-to-Command composition seam. It binds a fixed typed Command set to
/// one caller-owned Command::Runtime and delegates all WiFi behavior to the
/// supplied WiFiManager. It owns no Command queue, registry, retry engine,
/// response router, execution worker or transport state.
class WiFiCommandHandler final {
public:
    Command::CommandRuntimeStatus Bind(Command::Runtime& runtime, WiFiManager& manager) {
        if (_runtime == &runtime && _manager == &manager) return Command::CommandRuntimeStatus::Success;
        if (_runtime != nullptr || _manager != nullptr) return Command::CommandRuntimeStatus::Frozen;
        _manager = &manager;

#define ESPRESSIO_WIFI_BIND(Type, Method) \
    do { \
        const auto status = runtime.template BindHandler<Type>(*this, &WiFiCommandHandler::Method); \
        if (status != Command::CommandRuntimeStatus::Success) { _manager = nullptr; return status; } \
    } while (false)
        ESPRESSIO_WIFI_BIND(WiFiConfigureCommand, HandleConfigure);
        ESPRESSIO_WIFI_BIND(WiFiDisableCommand, HandleDisable);
        ESPRESSIO_WIFI_BIND(WiFiConnectClientCommand, HandleConnectClient);
        ESPRESSIO_WIFI_BIND(WiFiDisconnectClientCommand, HandleDisconnectClient);
        ESPRESSIO_WIFI_BIND(WiFiStartAccessPointCommand, HandleStartAccessPoint);
        ESPRESSIO_WIFI_BIND(WiFiStopAccessPointCommand, HandleStopAccessPoint);
        ESPRESSIO_WIFI_BIND(WiFiScanCommand, HandleScan);
        ESPRESSIO_WIFI_BIND(WiFiRetryKnownNetworksCommand, HandleRetryKnownNetworks);
        ESPRESSIO_WIFI_BIND(WiFiUpsertClientNetworkCommand, HandleUpsertClientNetwork);
        ESPRESSIO_WIFI_BIND(WiFiRemoveClientNetworkCommand, HandleRemoveClientNetwork);
        ESPRESSIO_WIFI_BIND(WiFiSetClientNetworkPriorityCommand, HandleSetClientNetworkPriority);
        ESPRESSIO_WIFI_BIND(WiFiSaveConfigurationCommand, HandleSaveConfiguration);
        ESPRESSIO_WIFI_BIND(WiFiLoadConfigurationCommand, HandleLoadConfiguration);
#undef ESPRESSIO_WIFI_BIND

        _runtime = &runtime;
        return Command::CommandRuntimeStatus::Success;
    }

    bool IsBound() const noexcept { return _runtime != nullptr && _manager != nullptr; }

private:
    WiFiCommandResponse HandleConfigure(const WiFiConfigureCommand& command, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->Configure(WiFiCommandDetail::ToNative(command.Configuration)));
    }
    WiFiCommandResponse HandleDisable(const WiFiDisableCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->Disable());
    }
    WiFiCommandResponse HandleConnectClient(const WiFiConnectClientCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->ConnectClient());
    }
    WiFiCommandResponse HandleDisconnectClient(const WiFiDisconnectClientCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->DisconnectClient());
    }
    WiFiCommandResponse HandleStartAccessPoint(const WiFiStartAccessPointCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->StartAccessPoint());
    }
    WiFiCommandResponse HandleStopAccessPoint(const WiFiStopAccessPointCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->StopAccessPoint());
    }
    WiFiCommandResponse HandleScan(const WiFiScanCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->Scan());
    }
    WiFiCommandResponse HandleRetryKnownNetworks(const WiFiRetryKnownNetworksCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStatus(_manager->RetryKnownNetworksNow());
    }
    WiFiCommandResponse HandleUpsertClientNetwork(const WiFiUpsertClientNetworkCommand& command, const Command::CommandExecutionContext&) {
        return _manager->AddOrUpdateClientNetwork(WiFiCommandDetail::ToNative(command.Profile))
            ? WiFiCommandResponse{WiFiCommandOutcome::Success}
            : WiFiCommandResponse{WiFiCommandOutcome::InvalidConfiguration};
    }
    WiFiCommandResponse HandleRemoveClientNetwork(const WiFiRemoveClientNetworkCommand& command, const Command::CommandExecutionContext&) {
        return _manager->RemoveClientNetwork(command.SSID.view())
            ? WiFiCommandResponse{WiFiCommandOutcome::Success}
            : WiFiCommandResponse{WiFiCommandOutcome::NetworkNotFound};
    }
    WiFiCommandResponse HandleSetClientNetworkPriority(const WiFiSetClientNetworkPriorityCommand& command, const Command::CommandExecutionContext&) {
        return _manager->SetClientNetworkPriority(command.SSID.view(), command.Priority)
            ? WiFiCommandResponse{WiFiCommandOutcome::Success}
            : WiFiCommandResponse{WiFiCommandOutcome::NetworkNotFound};
    }
    WiFiCommandResponse HandleSaveConfiguration(const WiFiSaveConfigurationCommand&, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStoreStatus(_manager->SaveConfiguration());
    }
    WiFiCommandResponse HandleLoadConfiguration(const WiFiLoadConfigurationCommand& command, const Command::CommandExecutionContext&) {
        return WiFiCommandDetail::FromStoreStatus(_manager->LoadConfiguration(command.Apply));
    }

    WiFiManager* _manager = nullptr;
    Command::Runtime* _runtime = nullptr;
};

/// Registers the complete fixed WiFi administrative Command set into a caller-
/// owned TypeDirectory. Directory capacity/freeze timing remain caller-owned.
template<std::size_t Capacity>
Primitive::TypeDirectoryRegistrationStatus RegisterWiFiCommandTypes(Primitive::TypeDirectory<Capacity>& directory) noexcept {
    using Status = Primitive::TypeDirectoryRegistrationStatus;
#define ESPRESSIO_WIFI_REGISTER_COMMAND(Type) \
    do { const auto status = directory.template Register<Type>(); if (status != Status::Success) return status; } while (false)
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiConfigureCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiDisableCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiConnectClientCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiDisconnectClientCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiStartAccessPointCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiStopAccessPointCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiScanCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiRetryKnownNetworksCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiUpsertClientNetworkCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiRemoveClientNetworkCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiSetClientNetworkPriorityCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiSaveConfigurationCommand);
    ESPRESSIO_WIFI_REGISTER_COMMAND(WiFiLoadConfigurationCommand);
#undef ESPRESSIO_WIFI_REGISTER_COMMAND
    return Status::Success;
}

} // namespace ESPressio::WiFi
