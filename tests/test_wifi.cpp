#include <cassert>
#include <vector>
#include <ESPressio_WiFi.hpp>

using namespace ESPressio::WiFi;

class FakePlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& config) override { configured=config; state.Mode=config.Mode; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus Disable() override { state.Mode=WiFiMode::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus ConnectClient() override { state.Client.State=ClientState::Connecting; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus DisconnectClient() override { state.Client.State=ClientState::Idle; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartAccessPoint() override { state.AccessPoint.State=AccessPointState::Active; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StopAccessPoint() override { state.AccessPoint.State=AccessPointState::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartScan() override { state.Scan=ScanState::Scanning; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus Poll(WiFiRuntimeState& output,std::vector<ScanResult>* scans,std::vector<WiFiPlatformEvent>* events) override {
        output=state;
        if (deliverScan && scans) { scans->push_back({"Lab",{},-42,6,NetworkSecurity::WPA2,false}); deliverScan=false; }
        if (!pending.empty() && events) { *events=pending; pending.clear(); }
        return WiFiStatus::Success;
    }
    WiFiConfiguration configured{}; WiFiRuntimeState state{}; bool deliverScan=false; std::vector<WiFiPlatformEvent> pending;
};

class Observer final : public IWiFiObserver {
public:
    void OnWiFiModeChanged(WiFiMode,WiFiMode) override { modes++; }
    void OnClientStateChanged(const ClientRuntimeState&,const ClientRuntimeState&) override { clients++; }
    void OnAccessPointStateChanged(const AccessPointRuntimeState&,const AccessPointRuntimeState&) override { aps++; }
    void OnScanCompleted(const std::vector<ScanResult>& r) override { scans += static_cast<int>(r.size()); }
    void OnAccessPointStationConnected(const MacAddress&) override { joined++; }
    void OnClientIPAddressAcquired(const NetworkAddress&) override { ips++; }
    int modes=0,clients=0,aps=0,scans=0,joined=0,ips=0;
};

int main() {
    FakePlatform platform; WiFiManager wifi(platform); Observer observer; auto handle=wifi.RegisterObserver(&observer); assert(handle);
    WiFiConfiguration config; config.Mode=WiFiMode::AccessPointClient; config.AccessPoint.SSID="lab-ap"; config.Client.SSID="lab-sta";
    assert(wifi.Configure(config)==WiFiStatus::Success); assert(wifi.Poll()==WiFiStatus::Success); assert(observer.modes==1);
    assert(wifi.ConnectClient()==WiFiStatus::Success); assert(wifi.Poll()==WiFiStatus::Success); assert(observer.clients==1);
    assert(wifi.StartAccessPoint()==WiFiStatus::Success); assert(wifi.Poll()==WiFiStatus::Success); assert(observer.aps==1);
    platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
    assert(wifi.Poll()==WiFiStatus::Success); assert(observer.scans==1);
    WiFiPlatformEvent joined{WiFiPlatformEventKind::AccessPointStationConnected,{},{}};
    WiFiPlatformEvent ip{WiFiPlatformEventKind::ClientIPAddressAcquired,{},{{IPv4Address(10,0,0,2),IPv4Address(10,0,0,1),IPv4Address(255,255,255,0),{}, {}}}};
    platform.pending={joined,ip}; assert(wifi.Poll()==WiFiStatus::Success); assert(observer.joined==1); assert(observer.ips==1);
    assert(wifi.Configuration().AccessPoint.SSID=="lab-ap");
    return 0;
}
