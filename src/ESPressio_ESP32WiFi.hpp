#pragma once

#if !defined(ARDUINO_ARCH_ESP32)
#error "ESPressio_ESP32WiFi.hpp requires an ESP32 Arduino target"
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <vector>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

class ESP32WiFiPlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& configuration) override {
        _configuration = configuration;
        wifi_mode_t mode = WIFI_MODE_NULL;
        switch (configuration.Mode) {
            case WiFiMode::Disabled: mode = WIFI_MODE_NULL; break;
            case WiFiMode::Client: mode = WIFI_MODE_STA; break;
            case WiFiMode::AccessPoint: mode = WIFI_MODE_AP; break;
            case WiFiMode::AccessPointClient: mode = WIFI_MODE_APSTA; break;
        }
        if (!::WiFi.mode(mode)) return WiFiStatus::PlatformError;
        if (!configuration.Hostname.empty()) (void)::WiFi.setHostname(configuration.Hostname.c_str());
        if (UsesAP(configuration.Mode) && !ConfigureAccessPoint()) return WiFiStatus::PlatformError;
        if (UsesClient(configuration.Mode) && !ConfigureClientAddress()) return WiFiStatus::PlatformError;
        ApplyRadioSettings();
        RefreshState();
        return WiFiStatus::Success;
    }

    WiFiStatus Disable() override {
        if (!::WiFi.mode(WIFI_MODE_NULL)) return WiFiStatus::PlatformError;
        _configuration.Mode = WiFiMode::Disabled;
        RefreshState();
        return WiFiStatus::Success;
    }

    WiFiStatus ConnectClient() override {
        if (_configuration.Client.SSID.empty()) return WiFiStatus::InvalidConfiguration;
        _state.Client.State = ClientState::Connecting;
        ++_state.Revision;
        ::WiFi.begin(_configuration.Client.SSID.c_str(),
            _configuration.Client.Password.empty() ? nullptr : _configuration.Client.Password.c_str());
        return WiFiStatus::Success;
    }

    WiFiStatus DisconnectClient() override {
        return ::WiFi.disconnect(false, false) ? WiFiStatus::Success : WiFiStatus::PlatformError;
    }

    WiFiStatus StartAccessPoint() override { return ConfigureAccessPoint() ? WiFiStatus::Success : WiFiStatus::PlatformError; }
    WiFiStatus StopAccessPoint() override { return ::WiFi.softAPdisconnect(false) ? WiFiStatus::Success : WiFiStatus::PlatformError; }

    WiFiStatus StartScan() override {
        if (_scanRunning) return WiFiStatus::Busy;
        const int16_t started = ::WiFi.scanNetworks(true, true);
        if (started == WIFI_SCAN_FAILED) return WiFiStatus::PlatformError;
        _scanRunning = true;
        _state.Scan = ScanState::Scanning;
        ++_state.Revision;
        return WiFiStatus::Success;
    }

    WiFiStatus Poll(WiFiRuntimeState& state, std::vector<ScanResult>* completedScan,
                    std::vector<WiFiPlatformEvent>* events) override {
        const auto before = _state;
        RefreshState();
        PollScan(completedScan);
        if (events != nullptr) BuildEvents(before, _state, *events);
        state = _state;
        return WiFiStatus::Success;
    }

private:
    static bool UsesAP(WiFiMode mode) { return mode == WiFiMode::AccessPoint || mode == WiFiMode::AccessPointClient; }
    static bool UsesClient(WiFiMode mode) { return mode == WiFiMode::Client || mode == WiFiMode::AccessPointClient; }
    static IPv4Address Convert(const IPAddress& v) { return IPv4Address(v[0], v[1], v[2], v[3]); }
    static IPAddress Convert(const IPv4Address& v) { return IPAddress(v.Octets[0], v.Octets[1], v.Octets[2], v.Octets[3]); }
    static bool IsZero(const IPv4Address& v) { return v.Octets[0]==0 && v.Octets[1]==0 && v.Octets[2]==0 && v.Octets[3]==0; }

    bool ConfigureClientAddress() {
        if (_configuration.Client.Addressing == AddressMode::DHCP) return ::WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
        const auto& n = _configuration.Client.StaticNetwork;
        return ::WiFi.config(Convert(n.Address), Convert(n.Gateway), Convert(n.SubnetMask), Convert(n.PrimaryDNS), Convert(n.SecondaryDNS));
    }

    bool ConfigureAccessPoint() {
        const auto& ap = _configuration.AccessPoint;
        if (ap.SSID.empty() || ap.Channel < 1 || ap.Channel > 14 || ap.MaximumClients == 0) return false;
        const auto& n = ap.Network;
        if (!::WiFi.softAPConfig(Convert(n.Address), Convert(n.Gateway), Convert(n.SubnetMask))) return false;
        const char* password = ap.Password.empty() ? nullptr : ap.Password.c_str();
        if (!::WiFi.softAP(ap.SSID.c_str(), password, ap.Channel, ap.Hidden, ap.MaximumClients)) return false;
        return true;
    }

    void ApplyRadioSettings() {
        int8_t dbm = _configuration.TxPowerDbm;
        if (dbm < 2) dbm = 2; if (dbm > 20) dbm = 20;
        (void)esp_wifi_set_max_tx_power(static_cast<int8_t>(dbm * 4));
        (void)esp_wifi_set_ps(_configuration.PowerSave ? WIFI_PS_MIN_MODEM : WIFI_PS_NONE);
    }

    void RefreshState() {
        WiFiRuntimeState next = _state;
        next.Mode = _configuration.Mode;
        if (UsesClient(_configuration.Mode)) {
            const auto status = ::WiFi.status();
            next.Client.State = status == WL_CONNECTED ? ClientState::Connected :
                (_state.Client.State == ClientState::Connecting ? ClientState::Connecting : ClientState::Idle);
            next.Client.SSID = std::string(::WiFi.SSID().c_str());
            next.Client.RSSI = ::WiFi.RSSI();
            next.Client.Channel = static_cast<uint8_t>(::WiFi.channel());
            next.Client.Network.Address = Convert(::WiFi.localIP());
            next.Client.Network.Gateway = Convert(::WiFi.gatewayIP());
            next.Client.Network.SubnetMask = Convert(::WiFi.subnetMask());
            next.Client.Network.PrimaryDNS = Convert(::WiFi.dnsIP(0));
            next.Client.Network.SecondaryDNS = Convert(::WiFi.dnsIP(1));
        } else { next.Client = ClientRuntimeState{}; }

        if (UsesAP(_configuration.Mode)) {
            next.AccessPoint.State = AccessPointState::Active;
            next.AccessPoint.SSID = _configuration.AccessPoint.SSID;
            next.AccessPoint.Channel = _configuration.AccessPoint.Channel;
            next.AccessPoint.Network = _configuration.AccessPoint.Network;
            next.AccessPoint.Network.Address = Convert(::WiFi.softAPIP());
            next.AccessPoint.ConnectedStations = static_cast<uint16_t>(::WiFi.softAPgetStationNum());
        } else { next.AccessPoint = AccessPointRuntimeState{}; }
        if (StateChanged(next, _state)) { next.Revision = _state.Revision + 1; _state = std::move(next); }
    }

    void PollScan(std::vector<ScanResult>* completedScan) {
        if (!_scanRunning) return;
        const int16_t count = ::WiFi.scanComplete();
        if (count == WIFI_SCAN_RUNNING) return;
        if (count < 0) { _scanRunning=false; _state.Scan=ScanState::Failed; ++_state.Revision; return; }
        if (completedScan != nullptr) {
            completedScan->clear(); completedScan->reserve(static_cast<std::size_t>(count));
            for (int16_t i=0; i<count; ++i) {
                ScanResult r; r.SSID=std::string(::WiFi.SSID(i).c_str()); r.RSSI=::WiFi.RSSI(i); r.Channel=static_cast<uint8_t>(::WiFi.channel(i));
                uint8_t* bssid=::WiFi.BSSID(i); if (bssid) for (std::size_t n=0;n<6;++n) r.BSSID.Octets[n]=bssid[n];
                r.Security=TranslateSecurity(::WiFi.encryptionType(i)); completedScan->push_back(std::move(r));
            }
        }
        ::WiFi.scanDelete(); _scanRunning=false; _state.Scan=ScanState::Complete; ++_state.Revision;
    }

    static void BuildEvents(const WiFiRuntimeState& before, const WiFiRuntimeState& after, std::vector<WiFiPlatformEvent>& events) {
        if (after.AccessPoint.ConnectedStations > before.AccessPoint.ConnectedStations)
            events.push_back({WiFiPlatformEventKind::AccessPointStationConnected, {}, {}});
        else if (after.AccessPoint.ConnectedStations < before.AccessPoint.ConnectedStations)
            events.push_back({WiFiPlatformEventKind::AccessPointStationDisconnected, {}, {}});
        const bool hadIP=!IsZero(before.Client.Network.Address), hasIP=!IsZero(after.Client.Network.Address);
        if (!hadIP && hasIP) events.push_back({WiFiPlatformEventKind::ClientIPAddressAcquired, {}, after.Client.Network});
        else if (hadIP && !hasIP) events.push_back({WiFiPlatformEventKind::ClientIPAddressLost, {}, {}});
    }

    static NetworkSecurity TranslateSecurity(wifi_auth_mode_t value) {
        switch(value) {
            case WIFI_AUTH_OPEN:return NetworkSecurity::Open; case WIFI_AUTH_WEP:return NetworkSecurity::WEP;
            case WIFI_AUTH_WPA_PSK:return NetworkSecurity::WPA; case WIFI_AUTH_WPA2_PSK:return NetworkSecurity::WPA2;
            case WIFI_AUTH_WPA_WPA2_PSK:return NetworkSecurity::WPA_WPA2;
#if defined(WIFI_AUTH_WPA3_PSK)
            case WIFI_AUTH_WPA3_PSK:return NetworkSecurity::WPA3;
#endif
#if defined(WIFI_AUTH_WPA2_WPA3_PSK)
            case WIFI_AUTH_WPA2_WPA3_PSK:return NetworkSecurity::WPA2_WPA3;
#endif
            default:return NetworkSecurity::Unknown;
        }
    }

    static bool StateChanged(const WiFiRuntimeState& a,const WiFiRuntimeState& b) {
        return a.Mode!=b.Mode || a.Client.State!=b.Client.State || a.Client.SSID!=b.Client.SSID || a.Client.RSSI!=b.Client.RSSI ||
            a.Client.Channel!=b.Client.Channel || a.Client.Network.Address!=b.Client.Network.Address ||
            a.AccessPoint.State!=b.AccessPoint.State || a.AccessPoint.ConnectedStations!=b.AccessPoint.ConnectedStations ||
            a.AccessPoint.Network.Address!=b.AccessPoint.Network.Address || a.Scan!=b.Scan;
    }

    WiFiConfiguration _configuration{};
    WiFiRuntimeState _state{};
    bool _scanRunning=false;
};

} // namespace ESPressio::WiFi
