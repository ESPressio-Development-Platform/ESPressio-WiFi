#pragma once

#include <ESPressio_Event.hpp>
#include "ESPressio_WiFiTypes.hpp"

namespace ESPressio::Event {

class WiFiModeChangedEvent final : public Event<> {
public:
    const WiFi::WiFiMode Before;
    const WiFi::WiFiMode After;
    WiFiModeChangedEvent(WiFi::WiFiMode before, WiFi::WiFiMode after) : Before(before), After(after) {}
};

class WiFiClientStateChangedEvent final : public Event<> {
public:
    const WiFi::ClientRuntimeState Before;
    const WiFi::ClientRuntimeState After;
    WiFiClientStateChangedEvent(const WiFi::ClientRuntimeState& before, const WiFi::ClientRuntimeState& after) : Before(before), After(after) {}
};

class WiFiAccessPointStateChangedEvent final : public Event<> {
public:
    const WiFi::AccessPointRuntimeState Before;
    const WiFi::AccessPointRuntimeState After;
    WiFiAccessPointStateChangedEvent(const WiFi::AccessPointRuntimeState& before, const WiFi::AccessPointRuntimeState& after) : Before(before), After(after) {}
};

class WiFiScanCompletedEvent final : public Event<> {
public:
    const std::vector<WiFi::ScanResult> Results;
    explicit WiFiScanCompletedEvent(const std::vector<WiFi::ScanResult>& results) : Results(results) {}
};

class WiFiAccessPointStationConnectedEvent final : public Event<> {
public: const WiFi::MacAddress Station; explicit WiFiAccessPointStationConnectedEvent(const WiFi::MacAddress& station) : Station(station) {}
};
class WiFiAccessPointStationDisconnectedEvent final : public Event<> {
public: const WiFi::MacAddress Station; explicit WiFiAccessPointStationDisconnectedEvent(const WiFi::MacAddress& station) : Station(station) {}
};
class WiFiClientIPAddressAcquiredEvent final : public Event<> {
public: const WiFi::NetworkAddress Network; explicit WiFiClientIPAddressAcquiredEvent(const WiFi::NetworkAddress& network) : Network(network) {}
};
class WiFiClientIPAddressLostEvent final : public Event<> {};

} // namespace ESPressio::Event
