#include <cassert>
#include <vector>
#include <ESPressio_WiFi.hpp>

using namespace ESPressio::WiFi;

class FakePlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& config) override { configured=config; state.Mode=config.Mode; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus Disable() override { state.Mode=WiFiMode::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus ConnectClient() override { state.Client.State=ClientState::Connecting; state.Revision++; legacyConnects++; return WiFiStatus::Success; }
    WiFiStatus ConnectClient(const ClientNetworkProfile& profile) override {
        connectedProfiles.push_back(profile.SSID); state.Client.State=ClientState::Connecting; state.Client.SSID=profile.SSID; state.Revision++; return WiFiStatus::Success;
    }
    WiFiStatus DisconnectClient() override { state.Client.State=ClientState::Disconnected; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartAccessPoint() override { state.AccessPoint.State=AccessPointState::Active; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StopAccessPoint() override { state.AccessPoint.State=AccessPointState::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartScan() override { state.Scan=ScanState::Scanning; state.Revision++; scanStarts++; return WiFiStatus::Success; }
    WiFiStatus Poll(WiFiRuntimeState& output,std::vector<ScanResult>* scans,std::vector<WiFiPlatformEvent>* events) override {
        output=state;
        if (deliverScan && scans) { *scans=nextScan; deliverScan=false; }
        if (!pending.empty() && events) { *events=pending; pending.clear(); }
        return WiFiStatus::Success;
    }
    WiFiConfiguration configured{};
    WiFiRuntimeState state{};
    bool deliverScan=false;
    int legacyConnects=0,scanStarts=0;
    std::vector<ScanResult> nextScan;
    std::vector<std::string> connectedProfiles;
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
    void OnScanCompleted(const std::vector<ScanResult>& r) override { scans += static_cast<int>(r.size()); }
    void OnClientNetworkSelectionChanged(const ClientNetworkSelectionRuntimeState&,const ClientNetworkSelectionRuntimeState&) override { selectionChanges++; }
    void OnClientNetworkSelected(const ClientNetworkCandidate& c) override { selected.push_back(c.SSID); selectedRSSI.push_back(c.RSSI); }
    void OnClientNoKnownNetworkAvailable() override { noKnown++; }
    int modes=0,clients=0,scans=0,selectionChanges=0,noKnown=0;
    std::vector<std::string> selected;
    std::vector<int32_t> selectedRSSI;
};

static ScanResult Visible(const char* ssid,int rssi,uint8_t channel) {
    ScanResult result; result.SSID=ssid; result.RSSI=rssi; result.Channel=channel; result.Security=NetworkSecurity::WPA2; return result;
}

int main() {
    FakePlatform platform;
    WiFiManager wifi(platform);
    Observer observer;
    auto handle=wifi.RegisterObserver(&observer);
    assert(handle);

    WiFiConfiguration config;
    config.Mode=WiFiMode::AccessPointClient;
    config.AccessPoint.SSID="lab-ap";
    config.Client.Enabled=true;
    config.Client.Selection.AutomaticSelection=true;
    config.Client.Selection.ScanOnStartup=true;
    config.Client.Selection.ScanOnDisconnect=true;
    config.Client.Selection.TryNextOnFailure=true;

    ClientNetworkProfile preferred; preferred.SSID="Preferred"; preferred.Password="preferred-pass"; preferred.Priority=300;
    ClientNetworkProfile backup; backup.SSID="Backup"; backup.Password="backup-pass"; backup.Priority=100;
    config.Client.Networks={backup,preferred};

    assert(wifi.Configure(config)==WiFiStatus::Success);
    assert(platform.scanStarts==1);
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(observer.modes==1);

    platform.nextScan={Visible("Backup",-30,1),Visible("Preferred",-70,11)};
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(platform.connectedProfiles.size()==1 && platform.connectedProfiles[0]=="Preferred");
    assert(observer.selected.back()=="Preferred");
    auto eligible=wifi.EligibleClientNetworks();
    assert(eligible.size()==2 && eligible[0].SSID=="Preferred" && eligible[1].SSID=="Backup");

    platform.state.Client.State=ClientState::Failed;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(platform.connectedProfiles.size()==2 && platform.connectedProfiles[1]=="Backup");

    platform.state.Client.State=ClientState::Connected;
    platform.state.Client.SSID="Backup";
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(wifi.State().Client.Selection.State==ClientNetworkSelectionState::Connected);

    // Healthy connections are sticky: a scan cannot force roaming to Preferred.
    platform.nextScan={Visible("Preferred",-20,6),Visible("Backup",-80,11)};
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    const auto connectsBeforeStickyScan=platform.connectedProfiles.size();
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(platform.connectedProfiles.size()==connectsBeforeStickyScan);

    // After disconnect, automatic selection runs again. Duplicate BSSIDs for one
    // SSID collapse to the strongest visible BSSID before priority ranking.
    platform.state.Client.State=ClientState::Disconnected;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(platform.scanStarts>=2);
    platform.nextScan={Visible("Preferred",-80,1),Visible("Preferred",-40,6),Visible("Backup",-20,11)};
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(observer.selected.back()=="Preferred");
    assert(observer.selectedRSSI.back()==-40);

    // Disconnect again and prove an unknown-only scan does not invent credentials.
    platform.state.Client.State=ClientState::Disconnected;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    platform.nextScan={Visible("Unknown",-10,6)};
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(observer.noKnown==1);
    assert(wifi.State().Client.Selection.State==ClientNetworkSelectionState::NoKnownNetworkAvailable);

    assert(wifi.SetClientNetworkPriority("Backup",500));
    assert(wifi.RemoveClientNetwork("Preferred"));
    ClientNetworkProfile replacement; replacement.SSID="Third"; replacement.Password="third-pass"; replacement.Priority=250;
    assert(wifi.AddOrUpdateClientNetwork(replacement));

    FakeStore store;
    wifi.SetConfigurationStore(&store);
    assert(wifi.SaveConfiguration());
    assert(store.saves==1 && store.saved.Client.Networks.size()==2);

    auto changed=wifi.Configuration(); changed.Hostname="changed";
    assert(wifi.Configure(changed)==WiFiStatus::Success);
    assert(wifi.LoadConfiguration(false));
    assert(store.loads==1);
    assert(wifi.Configuration().Hostname!="changed");
    return 0;
}
