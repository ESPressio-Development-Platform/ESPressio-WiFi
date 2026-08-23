#include <cassert>
#include <vector>
#include <ESPressio_WiFi.hpp>

using namespace ESPressio::WiFi;

class FakePlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& config) override { configured=config; state.Mode=config.Mode; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus Disable() override { state.Mode=WiFiMode::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus ConnectClient() override { state.Client.State=ClientState::Connecting; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus DisconnectClient() override { state.Client.State=ClientState::Disconnected; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartAccessPoint() override { state.AccessPoint.State=AccessPointState::Active; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StopAccessPoint() override { state.AccessPoint.State=AccessPointState::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartScan() override { state.Scan=ScanState::Scanning; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus Poll(WiFiRuntimeState& output,std::vector<ScanResult>* scans,std::vector<WiFiPlatformEvent>* events) override {
        output=state;
        if (deliverScan && scans) {
            ScanResult r; r.SSID="Lab"; r.RSSI=-42; r.Channel=6; r.Security=NetworkSecurity::WPA2;
            scans->push_back(r); deliverScan=false;
        }
        if (!pending.empty() && events) { *events=pending; pending.clear(); }
        return WiFiStatus::Success;
    }
    WiFiConfiguration configured{};
    WiFiRuntimeState state{};
    bool deliverScan=false;
    std::vector<WiFiPlatformEvent> pending;
};

class FakeStore final : public IWiFiConfigurationStore {
public:
    WiFiConfigurationStoreResult Save(const WiFiConfiguration& value) override { saved=value; has=true; saves++; return WiFiConfigurationStoreResult::Ok(); }
    WiFiConfigurationStoreResult Load(WiFiConfiguration& value) override {
        if (!has) return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::NotFound,"missing");
        value=saved; loads++; return WiFiConfigurationStoreResult::Ok();
    }
    WiFiConfiguration saved{}; bool has=false; int saves=0,loads=0;
};

class Observer final : public IWiFiObserver {
public:
    void OnWiFiModeChanged(WiFiMode,WiFiMode) override { modes++; }
    void OnClientStateChanged(const ClientRuntimeState&,const ClientRuntimeState&) override { clients++; }
    void OnAccessPointStateChanged(const AccessPointRuntimeState&,const AccessPointRuntimeState&) override { aps++; }
    void OnScanStateChanged(ScanState,ScanState) override { scanStates++; }
    void OnScanCompleted(const std::vector<ScanResult>& r) override { scans += static_cast<int>(r.size()); }
    void OnAccessPointStationConnected(const MacAddress&) override { joined++; }
    void OnAccessPointStationDisconnected(const MacAddress&) override { left++; }
    void OnClientIPAddressAcquired(const NetworkAddress&) override { ips++; }
    void OnClientIPAddressLost() override { ipLost++; }
    int modes=0,clients=0,aps=0,scanStates=0,scans=0,joined=0,left=0,ips=0,ipLost=0;
};

int main() {
    FakePlatform platform;
    WiFiManager wifi(platform);
    Observer observer;
    auto handle=wifi.RegisterObserver(&observer);
    assert(handle);

    int directScanStates=0,directIP=0,directLost=0;
    wifi.OnScanStateChanged([&](ScanState,ScanState){ directScanStates++; });
    wifi.OnClientIPAddressAcquired([&](const NetworkAddress&){ directIP++; });
    wifi.OnClientIPAddressLost([&](){ directLost++; });

    WiFiConfiguration config;
    assert(config.Mode==WiFiMode::AccessPoint);
    config.Mode=WiFiMode::AccessPointClient;
    config.AccessPoint.SSID="lab-ap";
    config.Client.Enabled=true;
    config.Client.SSID="lab-sta";
    config.AccessPoint.DHCP.LeaseStart=IPv4Address(192,168,4,20);
    config.Reconnect.BackoffMultiplier=2.0f;
    assert(wifi.Configure(config)==WiFiStatus::Success);
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.modes==1);

    assert(wifi.ConnectClient()==WiFiStatus::Success);
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.clients==1);

    assert(wifi.StartAccessPoint()==WiFiStatus::Success);
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.aps==1);

    assert(wifi.Scan()==WiFiStatus::Success);
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.scanStates>=1 && directScanStates>=1);
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.scans==1);
    assert(wifi.LastScanResults().size()==1);
    assert(wifi.LastScanResults()[0].SSID=="Lab");

    NetworkAddress network;
    network.Address=IPv4Address(10,0,0,2);
    network.Gateway=IPv4Address(10,0,0,1);
    network.SubnetMask=IPv4Address(255,255,255,0);
    WiFiPlatformEvent joined; joined.Kind=WiFiPlatformEventKind::AccessPointStationConnected;
    WiFiPlatformEvent ip; ip.Kind=WiFiPlatformEventKind::ClientIPAddressAcquired; ip.Network=network;
    platform.pending={joined,ip};
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.joined==1 && observer.ips==1 && directIP==1);

    WiFiPlatformEvent left; left.Kind=WiFiPlatformEventKind::AccessPointStationDisconnected;
    WiFiPlatformEvent lost; lost.Kind=WiFiPlatformEventKind::ClientIPAddressLost;
    platform.pending={left,lost};
    assert(wifi.Poll()==WiFiStatus::Success);
    assert(observer.left==1 && observer.ipLost==1 && directLost==1);

    FakeStore store;
    wifi.SetConfigurationStore(&store);
    assert(wifi.SaveConfiguration());
    assert(store.saves==1);
    auto changed=wifi.Configuration();
    changed.Hostname="changed";
    assert(wifi.Configure(changed)==WiFiStatus::Success);
    assert(wifi.LoadConfiguration(false));
    assert(store.loads==1);
    assert(wifi.Configuration().Hostname!="changed");

    assert(wifi.Configuration().AccessPoint.SSID=="lab-ap");
    assert(wifi.Configuration().AccessPoint.DHCP.LeaseStart==IPv4Address(192,168,4,20));
    return 0;
}
