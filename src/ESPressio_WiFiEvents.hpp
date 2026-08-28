#pragma once

#include <ESPressio_SerializableEvent.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::Event {

/// <summary>Serializable event describing an application-level Wi-Fi mode transition.</summary>
class WiFiModeChangedEvent final : public SerializableEvent<WiFiModeChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiModeChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::WiFiMode Before = WiFi::WiFiMode::Disabled;
    WiFi::WiFiMode After = WiFi::WiFiMode::Disabled;
    WiFiModeChangedEvent() = default;
    WiFiModeChangedEvent(WiFi::WiFiMode before, WiFi::WiFiMode after) : Before(before), After(after) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After))
};

/// <summary>Serializable event describing a client/station state transition and resulting network state.</summary>
class WiFiClientStateChangedEvent final : public SerializableEvent<WiFiClientStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::ClientState Before = WiFi::ClientState::Disabled;
    WiFi::ClientState After = WiFi::ClientState::Disabled;
    std::string SSID;
    WiFi::NetworkAddress Network{};
    WiFiClientStateChangedEvent() = default;
    WiFiClientStateChangedEvent(const WiFi::ClientRuntimeState& before, const WiFi::ClientRuntimeState& after)
        : Before(before.State), After(after.State), SSID(after.SSID), Network(after.Network) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("network", Network)
    )
};

/// <summary>Serializable event describing an access-point state transition.</summary>
class WiFiAccessPointStateChangedEvent final : public SerializableEvent<WiFiAccessPointStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::AccessPointState Before = WiFi::AccessPointState::Disabled;
    WiFi::AccessPointState After = WiFi::AccessPointState::Disabled;
    std::string SSID;
    uint16_t ConnectedStations = 0;
    WiFiAccessPointStateChangedEvent() = default;
    WiFiAccessPointStateChangedEvent(const WiFi::AccessPointRuntimeState& before, const WiFi::AccessPointRuntimeState& after)
        : Before(before.State), After(after.State), SSID(after.SSID), ConnectedStations(after.ConnectedStations) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("connectedStations", ConnectedStations)
    )
};

/// <summary>Serializable event describing an AP-until-client fallback state transition.</summary>
class WiFiAPUntilClientStateChangedEvent final : public SerializableEvent<WiFiAPUntilClientStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAPUntilClientStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::APUntilClientState Before = WiFi::APUntilClientState::Inactive;
    WiFi::APUntilClientState After = WiFi::APUntilClientState::Inactive;
    bool FallbackAccessPointActive = false;
    uint64_t FallbackDeadlineMilliseconds = 0;
    uint64_t NextRetryMilliseconds = 0;
    WiFiAPUntilClientStateChangedEvent() = default;
    WiFiAPUntilClientStateChangedEvent(
        const WiFi::APUntilClientRuntimeState& before,
        const WiFi::APUntilClientRuntimeState& after
    ) : Before(before.State), After(after.State), FallbackAccessPointActive(after.FallbackAccessPointActive),
        FallbackDeadlineMilliseconds(after.FallbackDeadlineMilliseconds), NextRetryMilliseconds(after.NextRetryMilliseconds) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("fallbackAccessPointActive", FallbackAccessPointActive),
        ESPRESSIO_PROPERTY("fallbackDeadlineMilliseconds", FallbackDeadlineMilliseconds),
        ESPRESSIO_PROPERTY("nextRetryMilliseconds", NextRetryMilliseconds)
    )
};

/// <summary>Serializable event describing a Wi-Fi scan-state transition.</summary>
class WiFiScanStateChangedEvent final : public SerializableEvent<WiFiScanStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::ScanState Before = WiFi::ScanState::Idle;
    WiFi::ScanState After = WiFi::ScanState::Idle;
    WiFiScanStateChangedEvent() = default;
    WiFiScanStateChangedEvent(WiFi::ScanState before, WiFi::ScanState after) : Before(before), After(after) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After))
};

/// <summary>Serializable event containing the completed Wi-Fi scan result set.</summary>
class WiFiScanCompletedEvent final : public SerializableEvent<WiFiScanCompletedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanCompletedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    std::vector<WiFi::ScanResult> Results;
    WiFiScanCompletedEvent() = default;
    explicit WiFiScanCompletedEvent(const std::vector<WiFi::ScanResult>& results) : Results(results) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("results", Results))
};

/// <summary>Serializable event identifying a station that connected to the local access point.</summary>
class WiFiAccessPointStationConnectedEvent final : public SerializableEvent<WiFiAccessPointStationConnectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStationConnectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::MacAddress Station{};
    WiFiAccessPointStationConnectedEvent() = default;
    explicit WiFiAccessPointStationConnectedEvent(const WiFi::MacAddress& station) : Station(station) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("station", Station))
};

/// <summary>Serializable event identifying a station that disconnected from the local access point.</summary>
class WiFiAccessPointStationDisconnectedEvent final : public SerializableEvent<WiFiAccessPointStationDisconnectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStationDisconnectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::MacAddress Station{};
    WiFiAccessPointStationDisconnectedEvent() = default;
    explicit WiFiAccessPointStationDisconnectedEvent(const WiFi::MacAddress& station) : Station(station) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("station", Station))
};

/// <summary>Serializable event reporting the network address acquired by the client interface.</summary>
class WiFiClientIPAddressAcquiredEvent final : public SerializableEvent<WiFiClientIPAddressAcquiredEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientIPAddressAcquiredEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::NetworkAddress Network{};
    WiFiClientIPAddressAcquiredEvent() = default;
    explicit WiFiClientIPAddressAcquiredEvent(const WiFi::NetworkAddress& network) : Network(network) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("network", Network))
};

/// <summary>Serializable event reporting loss of the client interface IP address.</summary>
class WiFiClientIPAddressLostEvent final : public SerializableEvent<WiFiClientIPAddressLostEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientIPAddressLostEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_SERIALIZABLE_PROPERTIES()
};

/// <summary>Serializable event describing preferred-client-network selection state and candidate metadata.</summary>
class WiFiClientNetworkSelectionChangedEvent final : public SerializableEvent<WiFiClientNetworkSelectionChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientNetworkSelectionChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::ClientNetworkSelectionState Before = WiFi::ClientNetworkSelectionState::Idle;
    WiFi::ClientNetworkSelectionState After = WiFi::ClientNetworkSelectionState::Idle;
    std::string SelectedSSID;
    uint16_t SelectedPriority = 0;
    uint32_t EligibleCandidateCount = 0;
    WiFiClientNetworkSelectionChangedEvent() = default;
    WiFiClientNetworkSelectionChangedEvent(
        const WiFi::ClientNetworkSelectionRuntimeState& before,
        const WiFi::ClientNetworkSelectionRuntimeState& after
    ) : Before(before.State), After(after.State), SelectedSSID(after.SelectedSSID),
        SelectedPriority(after.SelectedPriority), EligibleCandidateCount(static_cast<uint32_t>(after.EligibleCandidateCount)) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before), ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("selectedSSID", SelectedSSID), ESPRESSIO_PROPERTY("selectedPriority", SelectedPriority),
        ESPRESSIO_PROPERTY("eligibleCandidateCount", EligibleCandidateCount)
    )
};

/// <summary>Serializable event describing the known client-network candidate selected for connection.</summary>
class WiFiClientNetworkSelectedEvent final : public SerializableEvent<WiFiClientNetworkSelectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientNetworkSelectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    std::string SSID;
    WiFi::MacAddress BSSID{};
    uint16_t Priority = 0;
    int32_t RSSI = 0;
    uint8_t Channel = 0;
    WiFiClientNetworkSelectedEvent() = default;
    explicit WiFiClientNetworkSelectedEvent(const WiFi::ClientNetworkCandidate& candidate)
        : SSID(candidate.SSID), BSSID(candidate.BSSID), Priority(candidate.Priority), RSSI(candidate.RSSI), Channel(candidate.Channel) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("ssid", SSID), ESPRESSIO_PROPERTY("bssid", BSSID),
        ESPRESSIO_PROPERTY("priority", Priority), ESPRESSIO_PROPERTY("rssi", RSSI), ESPRESSIO_PROPERTY("channel", Channel)
    )
};

/// <summary>Serializable event reporting that no configured known client network is currently available.</summary>
class WiFiClientNoKnownNetworkAvailableEvent final : public SerializableEvent<WiFiClientNoKnownNetworkAvailableEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientNoKnownNetworkAvailableEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_SERIALIZABLE_PROPERTIES()
};

} // namespace ESPressio::Event
