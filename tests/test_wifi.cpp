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
    void OnAccessPointStateChanged(const AccessPointRuntimeState&,const AccessPointRuntimeState&) override { aps++; }
    void OnScanStateChanged(ScanState,ScanState) override { scanStates++; }
    void OnScanCompleted(const std::vector<ScanResult>& r) override { scans += static_cast<int>(r.size()); }
    void OnClientNetworkSelectionChanged(const ClientNetworkSelectionRuntimeState&,const ClientNetworkSelectionRuntimeState&) override { selectionChanges++; }
    void OnClientNetworkSelected(const ClientNetworkCandidate& c) override { selected.push_back(c.SSID); }
    void OnClientNoKnownNetworkAvailable() override { noKnown++; }
    int modes=0,clients=0,aps=0,scanStates=0,scans=0,selectionChanges=0,noKnown=0;
    std::vector<std::string> selected;
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
    config.Client.Selection.TryNextOnFailure=true;

    ClientNetworkProfile preferred; preferred.SSID="Preferred"; preferred.Password="preferred-pass"; preferred.Priority=300;
    ClientNetworkProfile backup; backup.SSID="Backup"; backup.Password="backup-pass"; backup.Priority=100;
    config.Client.Networks={backup,preferred}; // vector order deliberately opposes priority

    assert(wifi.Configure(config)==WiFiStatus::Success);
    assert(platform.scanStarts==1); // autonomous startup scan request
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(observer.modes==1);

    // Backup has the stronger RSSI, but Preferred wins because configured priority is authoritative.
    platform.nextScan={Visible("Backup",-30,1),Visible("Preferred",-70,11)};
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(platform.connectedProfiles.size()==1);
    assert(platform.connectedProfiles[0]=="Preferred");
    assert(observer.selected.back()=="Preferred");
    auto eligible=wifi.EligibleClientNetworks();
    assert(eligible.size()==2 && eligible[0].SSID=="Preferred" && eligible[1].SSID=="Backup");

    // A failed preferred connection advances to the next candidate without requiring application logic.
    platform.state.Client.State=ClientState::Failed;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(platform.connectedProfiles.size()==2);
    assert(platform.connectedProfiles[1]=="Backup");

    platform.state.Client.State=ClientState::Connected;
    platform.state.Client.SSID="Backup";
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    assert(wifi.State().Client.Selection.State==ClientNetworkSelectionState::Connected);

    // Duplicate BSSIDs for the same remembered SSID collapse to the strongest visible BSSID.
    platform.nextScan={Visible("Preferred",-80,1),Visible("Preferred",-40,6),Visible("Backup",-20,11)};
    platform.deliverScan=true;
    platform.state.Scan=ScanState::Complete;
    platform.state.Revision++;
    assert(wifi.ProcessOnce()==WiFiStatus::Success);
    eligible=wifi.EligibleClientNetworks();
    assert(eligible[0].SSID=="Preferred" && eligible[0].RSSI==-40 && eligible[0].Channel==6);

    // Unknown scans do not invent credentials and expose explicit no-known-network state.
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
    assert(store.saves==1);
    const auto saved=store.saved;
    assert(saved.Client.Networks.size()==2);
    assert(saved.Client.Networks[0].SSID=="Backup" || saved.Client.Networks[1].SSID=="Backup");

    auto changed=wifi.Configuration(); changed.Hostname="changed";
    assert(wifi.Configure(changed)==WiFiStatus::Success);
    assert(wifi.LoadConfiguration(false));
    assert(store.loads==1);
    assert(wifi.Configuration().Hostname!="changed");

    return 0;
}
