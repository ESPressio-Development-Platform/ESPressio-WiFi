#pragma once

#include <cstdint>
#include <limits>
#include <string_view>

#include <ESPressio_SerializableEvent.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::Event {

/// WiFi-owned bounded diagnostic payload limits. These limits apply only to
/// Primitive-family representations; WiFiManager retains its existing dynamic
/// application-owned storage policy.
inline constexpr std::size_t WiFiEventSSIDMaximumBytes = 32;
inline constexpr std::size_t WiFiEventMaximumScanResults = 16;

namespace WiFiEventDetail {

template<std::size_t N>
inline bool CopyText(
    Serializable::BoundedString<N>& destination,
    const WiFi::WiFiString& source
) noexcept {
    const auto view = source.empty()
        ? std::string_view{}
        : std::string_view(source.data(), source.size());
    if (destination.assign(view)) return false;
    (void)destination.assign(view.substr(0, N));
    return true;
}

inline std::uint32_t SaturatedCount(std::size_t value) noexcept {
    constexpr auto Maximum = std::numeric_limits<std::uint32_t>::max();
    return value > Maximum ? Maximum : static_cast<std::uint32_t>(value);
}

} // namespace WiFiEventDetail

/// Bounded serializable scan-result snapshot used only by the WiFi Event family
/// projection. It does not replace WiFiManager's native ScanResult storage.
struct WiFiScanResultSnapshot final : Serializable::Serializable<WiFiScanResultSnapshot> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanResultSnapshot)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    Serializable::BoundedString<WiFiEventSSIDMaximumBytes> SSID;
    bool SSIDTruncated = false;
    WiFi::MacAddress BSSID{};
    std::int32_t RSSI = 0;
    std::uint8_t Channel = 0;
    WiFi::NetworkSecurity Security = WiFi::NetworkSecurity::Unknown;
    bool Hidden = false;

    WiFiScanResultSnapshot() = default;
    explicit WiFiScanResultSnapshot(const WiFi::ScanResult& source) noexcept
        : SSIDTruncated(WiFiEventDetail::CopyText(SSID, source.SSID)),
          BSSID(source.BSSID),
          RSSI(source.RSSI),
          Channel(source.Channel),
          Security(source.Security),
          Hidden(source.Hidden) {}

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY("ssidTruncated", SSIDTruncated),
        ESPRESSIO_PROPERTY("bssid", BSSID),
        ESPRESSIO_PROPERTY("rssi", RSSI),
        ESPRESSIO_PROPERTY("channel", Channel),
        ESPRESSIO_PROPERTY("security", Security),
        ESPRESSIO_PROPERTY("hidden", Hidden)
    )
};

// Event TypeIds are explicit stable WiFi-owned assignments. They are deliberately
// not derived from CanonicalName: canonical-name hashing is not a semantic TypeId.

class WiFiModeChangedEvent final : public SerializableEvent<WiFiModeChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiModeChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010001ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.ModeChanged";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::WiFiMode Before = WiFi::WiFiMode::Disabled;
    WiFi::WiFiMode After = WiFi::WiFiMode::Disabled;
    WiFiModeChangedEvent() = default;
    WiFiModeChangedEvent(WiFi::WiFiMode before, WiFi::WiFiMode after) noexcept : Before(before), After(after) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After))
};

class WiFiClientStateChangedEvent final : public SerializableEvent<WiFiClientStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010002ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Client.StateChanged";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::ClientState Before = WiFi::ClientState::Disabled;
    WiFi::ClientState After = WiFi::ClientState::Disabled;
    Serializable::BoundedString<WiFiEventSSIDMaximumBytes> SSID;
    bool SSIDTruncated = false;
    WiFi::NetworkAddress Network{};

    WiFiClientStateChangedEvent() = default;
    WiFiClientStateChangedEvent(const WiFi::ClientRuntimeState& before, const WiFi::ClientRuntimeState& after) noexcept
        : Before(before.State), After(after.State),
          SSIDTruncated(WiFiEventDetail::CopyText(SSID, after.SSID)), Network(after.Network) {}

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("ssidTruncated", SSIDTruncated),
        ESPRESSIO_PROPERTY("network", Network)
    )
};

class WiFiAccessPointStateChangedEvent final : public SerializableEvent<WiFiAccessPointStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010003ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.AccessPoint.StateChanged";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::AccessPointState Before = WiFi::AccessPointState::Disabled;
    WiFi::AccessPointState After = WiFi::AccessPointState::Disabled;
    Serializable::BoundedString<WiFiEventSSIDMaximumBytes> SSID;
    bool SSIDTruncated = false;
    std::uint16_t ConnectedStations = 0;

    WiFiAccessPointStateChangedEvent() = default;
    WiFiAccessPointStateChangedEvent(const WiFi::AccessPointRuntimeState& before, const WiFi::AccessPointRuntimeState& after) noexcept
        : Before(before.State), After(after.State),
          SSIDTruncated(WiFiEventDetail::CopyText(SSID, after.SSID)), ConnectedStations(after.ConnectedStations) {}

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("ssidTruncated", SSIDTruncated),
        ESPRESSIO_PROPERTY("connectedStations", ConnectedStations)
    )
};

class WiFiAPUntilClientStateChangedEvent final : public SerializableEvent<WiFiAPUntilClientStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAPUntilClientStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010004ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.APUntilClient.StateChanged";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::APUntilClientState Before = WiFi::APUntilClientState::Inactive;
    WiFi::APUntilClientState After = WiFi::APUntilClientState::Inactive;
    bool FallbackAccessPointActive = false;
    std::uint64_t FallbackDeadlineMilliseconds = 0;
    std::uint64_t NextRetryMilliseconds = 0;

    WiFiAPUntilClientStateChangedEvent() = default;
    WiFiAPUntilClientStateChangedEvent(
        const WiFi::APUntilClientRuntimeState& before,
        const WiFi::APUntilClientRuntimeState& after
    ) noexcept : Before(before.State), After(after.State),
        FallbackAccessPointActive(after.FallbackAccessPointActive),
        FallbackDeadlineMilliseconds(after.FallbackDeadlineMilliseconds),
        NextRetryMilliseconds(after.NextRetryMilliseconds) {}

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("fallbackAccessPointActive", FallbackAccessPointActive),
        ESPRESSIO_PROPERTY("fallbackDeadlineMilliseconds", FallbackDeadlineMilliseconds),
        ESPRESSIO_PROPERTY("nextRetryMilliseconds", NextRetryMilliseconds)
    )
};

class WiFiScanStateChangedEvent final : public SerializableEvent<WiFiScanStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010005ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Scan.StateChanged";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::ScanState Before = WiFi::ScanState::Idle;
    WiFi::ScanState After = WiFi::ScanState::Idle;
    WiFiScanStateChangedEvent() = default;
    WiFiScanStateChangedEvent(WiFi::ScanState before, WiFi::ScanState after) noexcept : Before(before), After(after) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After))
};

class WiFiScanCompletedEvent final : public SerializableEvent<WiFiScanCompletedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanCompletedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010006ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Scan.Completed";
    static constexpr std::size_t MaximumLiveInstances = 2;
    static constexpr std::size_t MaximumPendingInstances = 2;

    Serializable::BoundedVector<WiFiScanResultSnapshot, WiFiEventMaximumScanResults> Results;
    std::uint32_t TotalResults = 0;
    bool Truncated = false;

    WiFiScanCompletedEvent() = default;
    explicit WiFiScanCompletedEvent(const WiFi::WiFiVector<WiFi::ScanResult>& results) noexcept
        : TotalResults(WiFiEventDetail::SaturatedCount(results.size())),
          Truncated(results.size() > WiFiEventMaximumScanResults) {
        const auto count = results.size() < WiFiEventMaximumScanResults
            ? results.size() : WiFiEventMaximumScanResults;
        for (std::size_t index = 0; index < count; ++index)
            (void)Results.push_back(WiFiScanResultSnapshot(results[index]));
    }

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("results", Results),
        ESPRESSIO_PROPERTY("totalResults", TotalResults),
        ESPRESSIO_PROPERTY("truncated", Truncated)
    )
};

class WiFiAccessPointStationConnectedEvent final : public SerializableEvent<WiFiAccessPointStationConnectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStationConnectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010007ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.AccessPoint.StationConnected";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::MacAddress Station{};
    WiFiAccessPointStationConnectedEvent() = default;
    explicit WiFiAccessPointStationConnectedEvent(const WiFi::MacAddress& station) noexcept : Station(station) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("station", Station))
};

class WiFiAccessPointStationDisconnectedEvent final : public SerializableEvent<WiFiAccessPointStationDisconnectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStationDisconnectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010008ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.AccessPoint.StationDisconnected";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::MacAddress Station{};
    WiFiAccessPointStationDisconnectedEvent() = default;
    explicit WiFiAccessPointStationDisconnectedEvent(const WiFi::MacAddress& station) noexcept : Station(station) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("station", Station))
};

class WiFiClientIPAddressAcquiredEvent final : public SerializableEvent<WiFiClientIPAddressAcquiredEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientIPAddressAcquiredEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x5749464900010009ULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Client.IPAddressAcquired";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::NetworkAddress Network{};
    WiFiClientIPAddressAcquiredEvent() = default;
    explicit WiFiClientIPAddressAcquiredEvent(const WiFi::NetworkAddress& network) noexcept : Network(network) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("network", Network))
};

class WiFiClientIPAddressLostEvent final : public SerializableEvent<WiFiClientIPAddressLostEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientIPAddressLostEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x574946490001000AULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Client.IPAddressLost";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;
    ESPRESSIO_SERIALIZABLE_PROPERTIES()
};

class WiFiClientNetworkSelectionChangedEvent final : public SerializableEvent<WiFiClientNetworkSelectionChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientNetworkSelectionChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x574946490001000BULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Client.NetworkSelectionChanged";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    WiFi::ClientNetworkSelectionState Before = WiFi::ClientNetworkSelectionState::Idle;
    WiFi::ClientNetworkSelectionState After = WiFi::ClientNetworkSelectionState::Idle;
    Serializable::BoundedString<WiFiEventSSIDMaximumBytes> SelectedSSID;
    bool SelectedSSIDTruncated = false;
    std::uint16_t SelectedPriority = 0;
    std::uint32_t EligibleCandidateCount = 0;

    WiFiClientNetworkSelectionChangedEvent() = default;
    WiFiClientNetworkSelectionChangedEvent(
        const WiFi::ClientNetworkSelectionRuntimeState& before,
        const WiFi::ClientNetworkSelectionRuntimeState& after
    ) noexcept : Before(before.State), After(after.State),
        SelectedSSIDTruncated(WiFiEventDetail::CopyText(SelectedSSID, after.SelectedSSID)),
        SelectedPriority(after.SelectedPriority),
        EligibleCandidateCount(WiFiEventDetail::SaturatedCount(after.EligibleCandidateCount)) {}

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("selectedSSID", SelectedSSID),
        ESPRESSIO_PROPERTY("selectedSSIDTruncated", SelectedSSIDTruncated),
        ESPRESSIO_PROPERTY("selectedPriority", SelectedPriority),
        ESPRESSIO_PROPERTY("eligibleCandidateCount", EligibleCandidateCount)
    )
};

class WiFiClientNetworkSelectedEvent final : public SerializableEvent<WiFiClientNetworkSelectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientNetworkSelectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x574946490001000CULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Client.NetworkSelected";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;

    Serializable::BoundedString<WiFiEventSSIDMaximumBytes> SSID;
    bool SSIDTruncated = false;
    WiFi::MacAddress BSSID{};
    std::uint16_t Priority = 0;
    std::int32_t RSSI = 0;
    std::uint8_t Channel = 0;

    WiFiClientNetworkSelectedEvent() = default;
    explicit WiFiClientNetworkSelectedEvent(const WiFi::ClientNetworkCandidate& candidate) noexcept
        : SSIDTruncated(WiFiEventDetail::CopyText(SSID, candidate.SSID)),
          BSSID(candidate.BSSID), Priority(candidate.Priority), RSSI(candidate.RSSI), Channel(candidate.Channel) {}

    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("ssidTruncated", SSIDTruncated),
        ESPRESSIO_PROPERTY("bssid", BSSID), ESPRESSIO_PROPERTY("priority", Priority),
        ESPRESSIO_PROPERTY("rssi", RSSI), ESPRESSIO_PROPERTY("channel", Channel)
    )
};

class WiFiClientNoKnownNetworkAvailableEvent final : public SerializableEvent<WiFiClientNoKnownNetworkAvailableEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientNoKnownNetworkAvailableEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    static constexpr EventTypeId TypeId{0x574946490001000DULL};
    static constexpr std::string_view CanonicalName = "ESPressio.WiFi.Client.NoKnownNetworkAvailable";
    static constexpr std::size_t MaximumLiveInstances = 4;
    static constexpr std::size_t MaximumPendingInstances = 8;
    ESPRESSIO_SERIALIZABLE_PROPERTIES()
};

/// Registers every WiFi-owned Event Type into one caller-owned, still-mutable
/// Primitive TypeDirectory. The caller owns directory capacity and freeze timing.
template<std::size_t Capacity>
Primitive::TypeDirectoryRegistrationStatus RegisterWiFiEventTypes(Primitive::TypeDirectory<Capacity>& directory) noexcept {
    using Status = Primitive::TypeDirectoryRegistrationStatus;
#define ESPRESSIO_WIFI_REGISTER_EVENT(Type) \
    do { const auto status = directory.template Register<Type>(); if (status != Status::Success) return status; } while (false)
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiModeChangedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiClientStateChangedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiAccessPointStateChangedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiAPUntilClientStateChangedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiScanStateChangedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiScanCompletedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiAccessPointStationConnectedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiAccessPointStationDisconnectedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiClientIPAddressAcquiredEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiClientIPAddressLostEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiClientNetworkSelectionChangedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiClientNetworkSelectedEvent);
    ESPRESSIO_WIFI_REGISTER_EVENT(WiFiClientNoKnownNetworkAvailableEvent);
#undef ESPRESSIO_WIFI_REGISTER_EVENT
    return Status::Success;
}

} // namespace ESPressio::Event
