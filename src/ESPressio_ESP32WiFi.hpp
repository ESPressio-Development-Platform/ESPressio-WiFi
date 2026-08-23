#pragma once

#if !defined(ARDUINO_ARCH_ESP32)
#error "ESPressio_ESP32WiFi.hpp requires an ESP32 Arduino target"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

class ESP32WiFiPlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& configuration) override {
        if (!Validate(configuration)) return WiFiStatus::InvalidConfiguration;
        _configuration = configuration;
        _manualDisconnect = false;
        _reconnectAttempts = 0;
        _nextReconnectMilliseconds = 0;
        _hasActiveProfile = false;

        wifi_mode_t mode = WIFI_MODE_NULL;
        switch (configuration.Mode) {
            case WiFiMode::Disabled: mode = WIFI_MODE_NULL; break;
            case WiFiMode::Client: mode = WIFI_MODE_STA; break;
            case WiFiMode::AccessPoint: mode = WIFI_MODE_AP; break;
            case WiFiMode::AccessPointClient:
            case WiFiMode::Provisioning:
                mode = WIFI_MODE_APSTA;
                break;
        }
        if (!::WiFi.mode(mode)) return WiFiStatus::PlatformError;
        if (!configuration.Hostname.empty()) (void)::WiFi.setHostname(configuration.Hostname.c_str());

        const bool automaticProfiles = UsesClient(configuration.Mode) && configuration.Client.Enabled &&
            configuration.Client.Selection.AutomaticSelection && !configuration.Client.Networks.empty();
        if (UsesClient(configuration.Mode) && !automaticProfiles && !ConfigureClientAddressLegacy()) return WiFiStatus::PlatformError;
        if (UsesAP(configuration.Mode) && !ConfigureAccessPoint()) return WiFiStatus::PlatformError;
        ApplyRadioSettings();

        if (UsesClient(configuration.Mode) && configuration.Client.Enabled &&
            !automaticProfiles && !configuration.Client.SSID.empty()) {
            BeginClient(false);
        }

        RefreshState();
        return WiFiStatus::Success;
    }

    WiFiStatus Disable() override {
        _manualDisconnect = true;
        if (!::WiFi.mode(WIFI_MODE_NULL)) return WiFiStatus::PlatformError;
        _configuration.Mode = WiFiMode::Disabled;
        _state.Client.State = ClientState::Disabled;
        _state.AccessPoint.State = AccessPointState::Disabled;
        ++_state.Revision;
        return WiFiStatus::Success;
    }

    WiFiStatus ConnectClient() override {
        if (!UsesClient(_configuration.Mode) || !_configuration.Client.Enabled || _configuration.Client.SSID.empty()) {
            return WiFiStatus::InvalidConfiguration;
        }
        _hasActiveProfile = false;
        if (!ConfigureClientAddressLegacy()) return WiFiStatus::PlatformError;
        _manualDisconnect = false;
        _reconnectAttempts = 0;
        BeginClient(false);
        return WiFiStatus::Success;
    }

    WiFiStatus ConnectClient(const ClientNetworkProfile& profile) override {
        if (!UsesClient(_configuration.Mode) || !_configuration.Client.Enabled || !profile.Enabled || profile.SSID.empty()) {
            return WiFiStatus::InvalidConfiguration;
        }
        if (!ValidateProfile(profile)) return WiFiStatus::InvalidConfiguration;
        _activeProfile = profile;
        _hasActiveProfile = true;
        if (!ConfigureClientAddress(profile.Addressing, profile.StaticNetwork)) return WiFiStatus::PlatformError;
        _manualDisconnect = false;
        _reconnectAttempts = 0;
        BeginClient(false);
        return WiFiStatus::Success;
    }

    WiFiStatus DisconnectClient() override {
        if (!UsesClient(_configuration.Mode)) return WiFiStatus::NotSupported;
        _manualDisconnect = true;
        _state.Client.State = ClientState::Disconnecting;
        ++_state.Revision;
        const bool disconnected = ::WiFi.disconnect(false, false);
        _state.Client.State = ClientState::Disconnected;
        ++_state.Revision;
        return disconnected ? WiFiStatus::Success : WiFiStatus::PlatformError;
    }

    WiFiStatus StartAccessPoint() override {
        if (!UsesAP(_configuration.Mode) || !_configuration.AccessPoint.Enabled) return WiFiStatus::NotSupported;
        _state.AccessPoint.State = AccessPointState::Starting;
        ++_state.Revision;
        if (!ConfigureAccessPoint()) {
            _state.AccessPoint.State = AccessPointState::Failed;
            ++_state.Revision;
            return WiFiStatus::PlatformError;
        }
        _state.AccessPoint.State = AccessPointState::Active;
        ++_state.Revision;
        return WiFiStatus::Success;
    }

    WiFiStatus StopAccessPoint() override {
        if (!UsesAP(_configuration.Mode)) return WiFiStatus::NotSupported;
        if (!::WiFi.softAPdisconnect(false)) return WiFiStatus::PlatformError;
        _state.AccessPoint.State = AccessPointState::Disabled;
        _knownStations.clear();
        ++_state.Revision;
        return WiFiStatus::Success;
    }

    WiFiStatus StartScan() override {
        if (_scanRunning) return WiFiStatus::Busy;
        const int16_t started = ::WiFi.scanNetworks(true, true);
        if (started == WIFI_SCAN_FAILED) {
            _state.Scan = ScanState::Failed;
            ++_state.Revision;
            return WiFiStatus::PlatformError;
        }
        _scanRunning = true;
        _state.Scan = ScanState::Scanning;
        ++_state.Revision;
        return WiFiStatus::Success;
    }

    WiFiStatus Poll(
        WiFiRuntimeState& state,
        std::vector<ScanResult>* completedScan,
        std::vector<WiFiPlatformEvent>* events
    ) override {
        const auto before = _state;
        PollReconnect();
        RefreshState();
        PollScan(completedScan);
        if (events != nullptr) {
            BuildNetworkEvents(before, _state, *events);
            BuildStationEvents(*events);
        }
        state = _state;
        return WiFiStatus::Success;
    }

private:
    static bool UsesAP(WiFiMode mode) {
        return mode == WiFiMode::AccessPoint ||
            mode == WiFiMode::AccessPointClient ||
            mode == WiFiMode::Provisioning;
    }

    static bool UsesClient(WiFiMode mode) {
        return mode == WiFiMode::Client ||
            mode == WiFiMode::AccessPointClient ||
            mode == WiFiMode::Provisioning;
    }

    static bool ValidateCredential(const std::string& password) {
        return password.empty() || (password.size() >= 8 && password.size() <= 63);
    }

    static bool ValidateProfile(const ClientNetworkProfile& profile) {
        return profile.SSID.size() <= 32 && ValidateCredential(profile.Password);
    }

    static bool Validate(const WiFiConfiguration& configuration) {
        if (configuration.Hostname.size() > 32) return false;
        if (configuration.AccessPoint.SSID.size() > 32 || configuration.Client.SSID.size() > 32) return false;
        if (!ValidateCredential(configuration.AccessPoint.Password) || !ValidateCredential(configuration.Client.Password)) return false;
        for (const auto& profile : configuration.Client.Networks) if (!ValidateProfile(profile)) return false;
        if (configuration.AccessPoint.Channel < 1 || configuration.AccessPoint.Channel > 14) return false;
        if (configuration.AccessPoint.MaximumClients == 0) return false;
        if (configuration.Reconnect.BackoffMultiplier < 1.0f) return false;
        if (configuration.Reconnect.MaximumDelayMilliseconds < configuration.Reconnect.InitialDelayMilliseconds) return false;
        return true;
    }

    static IPv4Address Convert(const IPAddress& value) { return IPv4Address(value[0], value[1], value[2], value[3]); }
    static IPAddress Convert(const IPv4Address& value) { return IPAddress(value.Octets[0], value.Octets[1], value.Octets[2], value.Octets[3]); }
    static bool IsZero(const IPv4Address& value) { return value.Octets[0] == 0 && value.Octets[1] == 0 && value.Octets[2] == 0 && value.Octets[3] == 0; }
    static bool TimeReached(uint32_t now, uint32_t target) { return static_cast<int32_t>(now - target) >= 0; }

    bool ConfigureClientAddress(AddressMode mode, const NetworkAddress& network) {
        if (mode == AddressMode::DHCP) return ::WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        return ::WiFi.config(Convert(network.Address), Convert(network.Gateway), Convert(network.SubnetMask), Convert(network.PrimaryDNS), Convert(network.SecondaryDNS));
    }

    bool ConfigureClientAddressLegacy() {
        return ConfigureClientAddress(_configuration.Client.Addressing, _configuration.Client.StaticNetwork);
    }

    bool ConfigureAccessPoint() {
        const auto& ap = _configuration.AccessPoint;
        if (!ap.Enabled || ap.SSID.empty()) return false;
        if (!::WiFi.softAPConfig(Convert(ap.Network.Address), Convert(ap.Network.Gateway), Convert(ap.Network.SubnetMask))) return false;
        const char* password = ap.Password.empty() ? nullptr : ap.Password.c_str();
        if (!::WiFi.softAP(ap.SSID.c_str(), password, ap.Channel, ap.Hidden, ap.MaximumClients)) return false;
        return ConfigureDHCPServer(ap.DHCP);
    }

    static bool ConfigureDHCPServer(const DHCPServerConfiguration& configuration) {
        esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (netif == nullptr) return false;
        (void)esp_netif_dhcps_stop(netif);
        if (!configuration.Enabled) return true;
#if defined(ESP_NETIF_REQUESTED_IP_ADDRESS)
        dhcps_lease_t lease{};
        lease.enable = true;
        IP4_ADDR(&lease.start_ip, configuration.LeaseStart.Octets[0], configuration.LeaseStart.Octets[1], configuration.LeaseStart.Octets[2], configuration.LeaseStart.Octets[3]);
        IP4_ADDR(&lease.end_ip, configuration.LeaseEnd.Octets[0], configuration.LeaseEnd.Octets[1], configuration.LeaseEnd.Octets[2], configuration.LeaseEnd.Octets[3]);
        if (esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_REQUESTED_IP_ADDRESS, &lease, sizeof(lease)) != ESP_OK) return false;
#endif
#if defined(ESP_NETIF_IP_ADDRESS_LEASE_TIME)
        uint32_t leaseMinutes = std::max<uint32_t>(1, configuration.LeaseDurationSeconds / 60);
        if (esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_IP_ADDRESS_LEASE_TIME, &leaseMinutes, sizeof(leaseMinutes)) != ESP_OK) return false;
#endif
        const esp_err_t started = esp_netif_dhcps_start(netif);
#if defined(ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED)
        return started == ESP_OK || started == ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED;
#else
        return started == ESP_OK;
#endif
    }

    void ApplyRadioSettings() {
        int8_t dbm = _configuration.TxPowerDbm;
        if (dbm < 2) dbm = 2;
        if (dbm > 20) dbm = 20;
        (void)esp_wifi_set_max_tx_power(static_cast<int8_t>(dbm * 4));
        (void)esp_wifi_set_ps(_configuration.PowerSave ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
    }

    const std::string& ActiveSSID() const { return _hasActiveProfile ? _activeProfile.SSID : _configuration.Client.SSID; }
    const std::string& ActivePassword() const { return _hasActiveProfile ? _activeProfile.Password : _configuration.Client.Password; }

    void BeginClient(bool reconnect) {
        const auto& ssid = ActiveSSID();
        const auto& password = ActivePassword();
        if (ssid.empty()) {
            _state.Client.State = ClientState::Failed;
            ++_state.Revision;
            return;
        }
        _state.Client.State = reconnect ? ClientState::Reconnecting : ClientState::Connecting;
        _state.Client.ReconnectAttempt = _reconnectAttempts;
        _clientAttemptStartedMilliseconds = millis();
        ++_state.Revision;
        ::WiFi.begin(ssid.c_str(), password.empty() ? nullptr : password.c_str());
    }

    void ScheduleReconnect() {
        if (_manualDisconnect || !_configuration.Reconnect.Enabled) {
            _state.Client.State = ClientState::Disconnected;
            ++_state.Revision;
            return;
        }
        if (_configuration.Reconnect.MaximumAttempts != 0 && _reconnectAttempts >= _configuration.Reconnect.MaximumAttempts) {
            _state.Client.State = ClientState::Failed;
            ++_state.Revision;
            return;
        }
        ++_reconnectAttempts;
        double delay = static_cast<double>(_configuration.Reconnect.InitialDelayMilliseconds);
        if (_reconnectAttempts > 1) delay *= std::pow(static_cast<double>(_configuration.Reconnect.BackoffMultiplier), static_cast<double>(_reconnectAttempts - 1));
        const uint32_t boundedDelay = static_cast<uint32_t>(std::min<double>(delay, static_cast<double>(_configuration.Reconnect.MaximumDelayMilliseconds)));
        _nextReconnectMilliseconds = millis() + boundedDelay;
        _state.Client.State = ClientState::Reconnecting;
        _state.Client.ReconnectAttempt = _reconnectAttempts;
        ++_state.Revision;
    }

    void PollReconnect() {
        if (!UsesClient(_configuration.Mode) || !_configuration.Client.Enabled || _manualDisconnect) return;
        if (ActiveSSID().empty()) return;
        const auto status = ::WiFi.status();
        if (status == WL_CONNECTED) {
            _reconnectAttempts = 0;
            _nextReconnectMilliseconds = 0;
            return;
        }
        if (_state.Client.State == ClientState::Connected) { ScheduleReconnect(); return; }
        if (_state.Client.State == ClientState::Connecting) {
            const uint32_t elapsed = millis() - _clientAttemptStartedMilliseconds;
            if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || elapsed >= _configuration.Reconnect.ConnectionTimeoutMilliseconds) ScheduleReconnect();
            return;
        }
        if (_state.Client.State == ClientState::Reconnecting && _nextReconnectMilliseconds != 0 && TimeReached(millis(), _nextReconnectMilliseconds)) {
            _nextReconnectMilliseconds = 0;
            BeginClient(true);
        }
    }

    void RefreshState() {
        WiFiRuntimeState next = _state;
        next.Mode = _configuration.Mode;
        if (UsesClient(_configuration.Mode)) {
            const auto status = ::WiFi.status();
            if (status == WL_CONNECTED) next.Client.State = ClientState::Connected;
            else if (_manualDisconnect) next.Client.State = ClientState::Disconnected;
            next.Client.SSID = status == WL_CONNECTED ? std::string(::WiFi.SSID().c_str()) : ActiveSSID();
            next.Client.RSSI = status == WL_CONNECTED ? ::WiFi.RSSI() : 0;
            next.Client.Channel = status == WL_CONNECTED ? static_cast<uint8_t>(::WiFi.channel()) : 0;
            next.Client.Network.Address = Convert(::WiFi.localIP());
            next.Client.Network.Gateway = Convert(::WiFi.gatewayIP());
            next.Client.Network.SubnetMask = Convert(::WiFi.subnetMask());
            next.Client.Network.PrimaryDNS = Convert(::WiFi.dnsIP(0));
            next.Client.Network.SecondaryDNS = Convert(::WiFi.dnsIP(1));
            next.Client.ReconnectAttempt = _reconnectAttempts;
        } else {
            next.Client = ClientRuntimeState{};
        }
        if (UsesAP(_configuration.Mode)) {
            const auto apIP = ::WiFi.softAPIP();
            if (apIP == IPAddress(0,0,0,0)) {
                next.AccessPoint.State = AccessPointState::Disabled;
                next.AccessPoint.Network.Address = IPv4Address{};
                next.AccessPoint.ConnectedStations = 0;
            } else {
                next.AccessPoint.State = AccessPointState::Active;
                next.AccessPoint.SSID = _configuration.AccessPoint.SSID;
                next.AccessPoint.Channel = _configuration.AccessPoint.Channel;
                next.AccessPoint.Network = _configuration.AccessPoint.Network;
                next.AccessPoint.Network.Address = Convert(apIP);
                next.AccessPoint.ConnectedStations = static_cast<uint16_t>(::WiFi.softAPgetStationNum());
            }
        } else next.AccessPoint = AccessPointRuntimeState{};
        if (StateChanged(next, _state)) {
            next.Revision = _state.Revision + 1;
            _state = std::move(next);
        }
    }

    void PollScan(std::vector<ScanResult>* completedScan) {
        if (!_scanRunning) return;
        const int16_t count = ::WiFi.scanComplete();
        if (count == WIFI_SCAN_RUNNING) return;
        if (count < 0) {
            _scanRunning = false;
            _state.Scan = ScanState::Failed;
            ++_state.Revision;
            return;
        }
        if (completedScan != nullptr) {
            completedScan->clear();
            completedScan->reserve(static_cast<std::size_t>(count));
            for (int16_t index = 0; index < count; ++index) {
                ScanResult result;
                result.SSID = std::string(::WiFi.SSID(index).c_str());
                result.RSSI = ::WiFi.RSSI(index);
                result.Channel = static_cast<uint8_t>(::WiFi.channel(index));
                uint8_t* bssid = ::WiFi.BSSID(index);
                if (bssid != nullptr) for (std::size_t octet = 0; octet < 6; ++octet) result.BSSID.Octets[octet] = bssid[octet];
                result.Security = TranslateSecurity(::WiFi.encryptionType(index));
                completedScan->push_back(std::move(result));
            }
        }
        ::WiFi.scanDelete();
        _scanRunning = false;
        _state.Scan = ScanState::Complete;
        ++_state.Revision;
    }

    static void BuildNetworkEvents(const WiFiRuntimeState& before, const WiFiRuntimeState& after, std::vector<WiFiPlatformEvent>& events) {
        const bool hadIP = !IsZero(before.Client.Network.Address);
        const bool hasIP = !IsZero(after.Client.Network.Address);
        if (!hadIP && hasIP) events.push_back({WiFiPlatformEventKind::ClientIPAddressAcquired, {}, after.Client.Network});
        else if (hadIP && !hasIP) events.push_back({WiFiPlatformEventKind::ClientIPAddressLost, {}, {}});
    }

    void BuildStationEvents(std::vector<WiFiPlatformEvent>& events) {
        std::vector<MacAddress> current;
        if (UsesAP(_configuration.Mode)) {
            wifi_sta_list_t stations{};
            if (esp_wifi_ap_get_sta_list(&stations) == ESP_OK) {
                current.reserve(stations.num);
                for (int index = 0; index < stations.num; ++index) {
                    MacAddress mac;
                    for (std::size_t octet = 0; octet < 6; ++octet) mac.Octets[octet] = stations.sta[index].mac[octet];
                    current.push_back(mac);
                }
            }
        }
        for (const auto& station : current) if (std::find(_knownStations.begin(), _knownStations.end(), station) == _knownStations.end()) events.push_back({WiFiPlatformEventKind::AccessPointStationConnected, station, {}});
        for (const auto& station : _knownStations) if (std::find(current.begin(), current.end(), station) == current.end()) events.push_back({WiFiPlatformEventKind::AccessPointStationDisconnected, station, {}});
        _knownStations = std::move(current);
    }

    static NetworkSecurity TranslateSecurity(wifi_auth_mode_t value) {
        switch (value) {
            case WIFI_AUTH_OPEN: return NetworkSecurity::Open;
            case WIFI_AUTH_WEP: return NetworkSecurity::WEP;
            case WIFI_AUTH_WPA_PSK: return NetworkSecurity::WPA;
            case WIFI_AUTH_WPA2_PSK: return NetworkSecurity::WPA2;
            case WIFI_AUTH_WPA_WPA2_PSK: return NetworkSecurity::WPA_WPA2;
#if defined(WIFI_AUTH_WPA3_PSK)
            case WIFI_AUTH_WPA3_PSK: return NetworkSecurity::WPA3;
#endif
#if defined(WIFI_AUTH_WPA2_WPA3_PSK)
            case WIFI_AUTH_WPA2_WPA3_PSK: return NetworkSecurity::WPA2_WPA3;
#endif
            default: return NetworkSecurity::Unknown;
        }
    }

    static bool StateChanged(const WiFiRuntimeState& a, const WiFiRuntimeState& b) {
        return a.Mode != b.Mode || a.Client.State != b.Client.State || a.Client.SSID != b.Client.SSID ||
            a.Client.RSSI != b.Client.RSSI || a.Client.Channel != b.Client.Channel ||
            a.Client.Network.Address != b.Client.Network.Address || a.Client.ReconnectAttempt != b.Client.ReconnectAttempt ||
            a.AccessPoint.State != b.AccessPoint.State || a.AccessPoint.ConnectedStations != b.AccessPoint.ConnectedStations ||
            a.AccessPoint.Network.Address != b.AccessPoint.Network.Address || a.Scan != b.Scan;
    }

    WiFiConfiguration _configuration{};
    WiFiRuntimeState _state{};
    ClientNetworkProfile _activeProfile{};
    bool _hasActiveProfile = false;
    bool _scanRunning = false;
    bool _manualDisconnect = false;
    uint32_t _clientAttemptStartedMilliseconds = 0;
    uint32_t _nextReconnectMilliseconds = 0;
    uint32_t _reconnectAttempts = 0;
    std::vector<MacAddress> _knownStations;
};

} // namespace ESPressio::WiFi
