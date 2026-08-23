#pragma once

#include <ESPressio_SerializableEvent.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::Event {

class WiFiModeChangedEvent final : public SerializableEvent<WiFiModeChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiModeChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::WiFiMode Before = WiFi::WiFiMode::Disabled;
    WiFi::WiFiMode After = WiFi::WiFiMode::Disabled;
    WiFiModeChangedEvent() = default;
    WiFiModeChangedEvent(WiFi::WiFiMode before, WiFi::WiFiMode after) : Before(before), After(after) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before),
        ESPRESSIO_PROPERTY("after", After)
    )
};

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
        ESPRESSIO_PROPERTY("before", Before),
        ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY("network", Network)
    )
};

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
        ESPRESSIO_PROPERTY("before", Before),
        ESPRESSIO_PROPERTY("after", After),
        ESPRESSIO_PROPERTY("ssid", SSID),
        ESPRESSIO_PROPERTY("connectedStations", ConnectedStations)
    )
};

class WiFiScanStateChangedEvent final : public SerializableEvent<WiFiScanStateChangedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanStateChangedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::ScanState Before = WiFi::ScanState::Idle;
    WiFi::ScanState After = WiFi::ScanState::Idle;
    WiFiScanStateChangedEvent() = default;
    WiFiScanStateChangedEvent(WiFi::ScanState before, WiFi::ScanState after) : Before(before), After(after) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(
        ESPRESSIO_PROPERTY("before", Before),
        ESPRESSIO_PROPERTY("after", After)
    )
};

class WiFiScanCompletedEvent final : public SerializableEvent<WiFiScanCompletedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiScanCompletedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    std::vector<WiFi::ScanResult> Results;
    WiFiScanCompletedEvent() = default;
    explicit WiFiScanCompletedEvent(const std::vector<WiFi::ScanResult>& results) : Results(results) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("results", Results))
};

class WiFiAccessPointStationConnectedEvent final : public SerializableEvent<WiFiAccessPointStationConnectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStationConnectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::MacAddress Station{};
    WiFiAccessPointStationConnectedEvent() = default;
    explicit WiFiAccessPointStationConnectedEvent(const WiFi::MacAddress& station) : Station(station) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("station", Station))
};

class WiFiAccessPointStationDisconnectedEvent final : public SerializableEvent<WiFiAccessPointStationDisconnectedEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiAccessPointStationDisconnectedEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::MacAddress Station{};
    WiFiAccessPointStationDisconnectedEvent() = default;
    explicit WiFiAccessPointStationDisconnectedEvent(const WiFi::MacAddress& station) : Station(station) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("station", Station))
};

class WiFiClientIPAddressAcquiredEvent final : public SerializableEvent<WiFiClientIPAddressAcquiredEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientIPAddressAcquiredEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    WiFi::NetworkAddress Network{};
    WiFiClientIPAddressAcquiredEvent() = default;
    explicit WiFiClientIPAddressAcquiredEvent(const WiFi::NetworkAddress& network) : Network(network) {}
    ESPRESSIO_SERIALIZABLE_PROPERTIES(ESPRESSIO_PROPERTY("network", Network))
};

class WiFiClientIPAddressLostEvent final : public SerializableEvent<WiFiClientIPAddressLostEvent> {
    ESPRESSIO_SERIALIZABLE_TYPE(WiFiClientIPAddressLostEvent)
    ESPRESSIO_SERIALIZABLE_SCHEMA_VERSION(1)
public:
    ESPRESSIO_SERIALIZABLE_PROPERTIES()
};

} // namespace ESPressio::Event
