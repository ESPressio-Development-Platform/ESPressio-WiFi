#include <cassert>
#include <vector>
#include <ESPressio_WiFi.hpp>

using namespace ESPressio::WiFi;

class FakePlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& config) override { configured=config; state.Mode=config.Mode; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus Disable() override { state.Mode=WiFiMode::Disabled; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus ConnectClient() override { state.Client.State=ClientState::Connecting; state.Revision++; legacyConnects++; return WiFiStatus::Success; }
    WiFiStatus ConnectClient(const ClientNetworkProfile& profile) override { connectedProfiles.push_back(profile.SSID); state.Client.State=ClientState::Connecting; state.Client.SSID=profile.SSID; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus DisconnectClient() override { state.Client.State=ClientState::Disconnected; state.Revision++; return WiFiStatus::Success; }
    WiFiStatus StartAccessPoint() override { state.AccessPoint.State=AccessPointState::Active; state.Revision++; apStarts++; return WiFiStatus::Success; }
    WiFiStatus StopAccessPoint() override { state.AccessPoint.State=AccessPointState::Disabled; state.Revision++; apStops++; return WiFiStatus::Success; }
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
    int legacyConnects=0,scanStarts=0,apStarts=0,apStops=0;
    std::vector<ScanResult> nextScan;
    std::vector<std::string> connectedProfiles;
    std::vector<WiFiPlatformEvent> pending;
};

class FakeStore final : public IWiFiConfigurationStore {
public:
    WiFiConfigurationStoreResult Save(const WiFiConfiguration& value) override { saved=value; has=true; saves++; return WiFiConfigurationStoreResult::Ok(); }
    WiFiConfigurationStoreResult Load(WiFiConfiguration& value) override { if (!has) return WiFiConfigurationStoreResult::Fail(WiFiConfigurationStoreStatus::NotFound,"missing"); value=saved; loads++; return WiFiConfigurationStoreResult::Ok(); }
    WiFiConfiguration saved{}; bool has=false; int saves=0,loads=0;
};

class Observer final : public IWiFiObserver {
public:
    void OnWiFiModeChanged(WiFiMode,WiFiMode) override { modes++; }
    void OnClientStateChanged(const ClientRuntimeState&,const ClientRuntimeState&) override { clients++; }
    void OnAPUntilClientStateChanged(const APUntilClientRuntimeState&,const APUntilClientRuntimeState& after) override { apUntilClientChanges++; apUntilClientStates.push_back(after.State); }
    void OnScanCompleted(const std::vector<ScanResult>& r) override { scanCompletions++; scans += static_cast<int>(r.size()); }
    void OnClientNetworkSelectionChanged(const ClientNetworkSelectionRuntimeState&,const ClientNetworkSelectionRuntimeState&) override { selectionChanges++; }
    void OnClientNetworkSelected(const ClientNetworkCandidate& c) override { selected.push_back(c.SSID); selectedRSSI.push_back(c.RSSI); }
    void OnClientNoKnownNetworkAvailable() override { noKnown++; }
    int modes=0,clients=0,scanCompletions=0,scans=0,selectionChanges=0,noKnown=0,apUntilClientChanges=0;
    std::vector<std::string> selected;
    std::vector<int32_t> selectedRSSI;
    std::vector<APUntilClientState> apUntilClientStates;
};

static ScanResult Visible(const char* ssid,int rssi,uint8_t channel) {
    ScanResult result; result.SSID=ssid; result.RSSI=rssi; result.Channel=channel; result.Security=NetworkSecurity::WPA2; return result;
}

static WiFiConfiguration AutomaticConfig(WiFiMode mode=WiFiMode::AccessPointClient) {
    WiFiConfiguration config;
    config.Mode=mode;
    config.AccessPoint.SSID="lab-ap";
    config.AccessPoint.Password="password123";
    config.Client.Enabled=true;
    config.Client.Selection.AutomaticSelection=true;
    config.Client.Selection.ScanOnStartup=true;
    config.Client.Selection.ScanOnDisconnect=true;
    config.Client.Selection.TryNextOnFailure=true;
    return config;
}

int main() {
    {
        FakePlatform platform;
        WiFiManager wifi(platform);
        Observer observer;
        auto handle=wifi.RegisterObserver(&observer);
        assert(handle);

        auto config=AutomaticConfig();
        ClientNetworkProfile preferred; preferred.SSID="Preferred"; preferred.Password="preferred-pass"; preferred.Priority=300;
        ClientNetworkProfile backup; backup.SSID="Backup"; backup.Password="backup-pass"; backup.Priority=100;
        config.Client.Networks={backup,preferred};

        assert(wifi.Configure(config)==WiFiStatus::Success);
        assert(platform.scanStarts==1);
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(observer.modes==1);

        platform.nextScan={Visible("Backup",-30,1),Visible("Preferred",-70,11)};
        platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.connectedProfiles.size()==1 && platform.connectedProfiles[0]=="Preferred");
        assert(observer.selected.back()=="Preferred");
        auto eligible=wifi.EligibleClientNetworks();
        assert(eligible.size()==2 && eligible[0].SSID=="Preferred" && eligible[1].SSID=="Backup");

        platform.state.Client.State=ClientState::Failed; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.connectedProfiles.size()==2 && platform.connectedProfiles[1]=="Backup");

        platform.state.Client.State=ClientState::Connected; platform.state.Client.SSID="Backup"; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(wifi.State().Client.Selection.State==ClientNetworkSelectionState::Connected);

        assert(wifi.Scan()==WiFiStatus::Success);
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        platform.nextScan={Visible("Preferred",-20,6),Visible("Backup",-80,11)};
        platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        const auto connectsBeforeStickyScan=platform.connectedProfiles.size();
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.connectedProfiles.size()==connectsBeforeStickyScan);

        platform.state.Client.State=ClientState::Disconnected; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.scanStarts>=3);
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        platform.nextScan={Visible("Preferred",-80,1),Visible("Preferred",-40,6),Visible("Backup",-20,11)};
        platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(observer.selected.back()=="Preferred");
        assert(observer.selectedRSSI.back()==-40);

        platform.state.Client.State=ClientState::Disconnected; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        platform.nextScan.clear(); platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        const int completionsBeforeEmpty=observer.scanCompletions;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(observer.scanCompletions==completionsBeforeEmpty+1);
        assert(observer.noKnown==1);
        assert(wifi.LastScanResults().empty());
        assert(wifi.State().Client.Selection.State==ClientNetworkSelectionState::NoKnownNetworkAvailable);

        assert(wifi.SetClientNetworkPriority("Backup",500));
        assert(wifi.RemoveClientNetwork("Preferred"));
        ClientNetworkProfile replacement; replacement.SSID="Third"; replacement.Password="third-pass"; replacement.Priority=250;
        assert(wifi.AddOrUpdateClientNetwork(replacement));

        FakeStore store; wifi.SetConfigurationStore(&store);
        assert(wifi.SaveConfiguration());
        assert(store.saves==1 && store.saved.Client.Networks.size()==2);
        auto changed=wifi.Configuration(); changed.Hostname="changed";
        assert(wifi.Configure(changed)==WiFiStatus::Success);
        assert(wifi.LoadConfiguration(false));
        assert(store.loads==1);
        assert(wifi.Configuration().Hostname!="changed");
    }

    // #15: no remembered networks means immediate AP fallback and emits both
    // the direct callback and Observer lifecycle notification.
    {
        uint64_t now=1000;
        FakePlatform platform;
        WiFiManager wifi(platform,[&](){return now;});
        Observer observer;
        auto handle=wifi.RegisterObserver(&observer); assert(handle);
        int directChanges=0;
        APUntilClientState directState=APUntilClientState::Inactive;
        wifi.OnAPUntilClientStateChanged([&](const APUntilClientRuntimeState&,const APUntilClientRuntimeState& after){directChanges++;directState=after.State;});
        auto config=AutomaticConfig(WiFiMode::APUntilClient);
        config.APUntilClient.FallbackTimeoutMilliseconds=5000;
        config.APUntilClient.RetryScanIntervalMilliseconds=2000;
        assert(wifi.Configure(config)==WiFiStatus::Success);
        assert(platform.apStarts==1);
        assert(platform.scanStarts==0);
        assert(wifi.State().APUntilClient.State==APUntilClientState::FallbackAccessPoint);
        assert(wifi.State().APUntilClient.FallbackAccessPointActive);
        assert(observer.apUntilClientChanges>=1);
        assert(observer.apUntilClientStates.back()==APUntilClientState::FallbackAccessPoint);
        assert(directChanges>=1 && directState==APUntilClientState::FallbackAccessPoint);
        assert(wifi.RetryKnownNetworksNow()==WiFiStatus::InvalidConfiguration);
    }

    // #15: remembered networks start STA-first; fallback waits for timeout,
    // retries while AP+STA, then returns to ClientConnected and can recover again
    // after a later Client loss.
    {
        uint64_t now=1000;
        FakePlatform platform;
        WiFiManager wifi(platform,[&](){return now;});
        Observer observer;
        auto handle=wifi.RegisterObserver(&observer); assert(handle);
        int directChanges=0;
        wifi.OnAPUntilClientStateChanged([&](const APUntilClientRuntimeState&,const APUntilClientRuntimeState&){directChanges++;});
        auto config=AutomaticConfig(WiFiMode::APUntilClient);
        config.APUntilClient.FallbackTimeoutMilliseconds=5000;
        config.APUntilClient.RetryScanIntervalMilliseconds=2000;
        ClientNetworkProfile known; known.SSID="Known"; known.Password="password123"; config.Client.Networks={known};
        assert(wifi.Configure(config)==WiFiStatus::Success);
        assert(platform.scanStarts==1);
        assert(platform.apStarts==0);
        assert(wifi.State().APUntilClient.State==APUntilClientState::SeekingClient);
        assert(wifi.State().APUntilClient.FallbackDeadlineMilliseconds==6000);

        platform.nextScan.clear(); platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.apStarts==0);

        now=5999; assert(wifi.ProcessOnce()==WiFiStatus::Success); assert(platform.apStarts==0);
        now=6000; assert(wifi.ProcessOnce()==WiFiStatus::Success); assert(platform.apStarts==1);
        assert(wifi.State().APUntilClient.State==APUntilClientState::FallbackAccessPoint);
        assert(wifi.State().APUntilClient.NextRetryMilliseconds==8000);

        const int scansAtFallback=platform.scanStarts;
        now=7999; assert(wifi.ProcessOnce()==WiFiStatus::Success); assert(platform.scanStarts==scansAtFallback);
        now=8000; assert(wifi.ProcessOnce()==WiFiStatus::Success); assert(platform.scanStarts==scansAtFallback+1);
        assert(wifi.State().APUntilClient.NextRetryMilliseconds==10000);

        const int scansBeforeExplicitRetry=platform.scanStarts;
        assert(wifi.RetryKnownNetworksNow()==WiFiStatus::Success);
        assert(platform.scanStarts==scansBeforeExplicitRetry+1);

        ClientNetworkProfile newKnown; newKnown.SSID="NewKnown"; newKnown.Password="password123"; newKnown.Priority=500;
        const int scansBeforeAdd=platform.scanStarts;
        assert(wifi.AddOrUpdateClientNetwork(newKnown));
        assert(platform.scanStarts==scansBeforeAdd+1);

        platform.state.Client.State=ClientState::Connected; platform.state.Client.SSID="NewKnown"; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.apStops==1);
        assert(wifi.State().APUntilClient.State==APUntilClientState::ClientConnected);
        assert(!wifi.State().APUntilClient.FallbackAccessPointActive);

        // A later loss of the established Client must arm a fresh fallback window.
        now=12000;
        platform.state.Client.State=ClientState::Disconnected; platform.state.Revision++;
        const int scansBeforeLoss=platform.scanStarts;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(wifi.State().APUntilClient.State==APUntilClientState::SeekingClient);
        assert(wifi.State().APUntilClient.FallbackDeadlineMilliseconds==17000);
        assert(platform.scanStarts>=scansBeforeLoss+1);
        now=16999; assert(wifi.ProcessOnce()==WiFiStatus::Success); assert(platform.apStarts==1);
        now=17000; assert(wifi.ProcessOnce()==WiFiStatus::Success); assert(platform.apStarts==2);

        assert(observer.apUntilClientChanges>=4);
        assert(directChanges==observer.apUntilClientChanges);
    }

    // #15: persistent AP+Client semantics remain unchanged.
    {
        uint64_t now=0;
        FakePlatform platform;
        WiFiManager wifi(platform,[&](){return now;});
        auto config=AutomaticConfig(WiFiMode::AccessPointClient);
        ClientNetworkProfile known; known.SSID="Known"; known.Password="password123"; config.Client.Networks={known};
        assert(wifi.Configure(config)==WiFiStatus::Success);
        platform.state.AccessPoint.State=AccessPointState::Active;
        platform.state.Client.State=ClientState::Connected;
        platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.apStops==0);
        assert(wifi.State().APUntilClient.State==APUntilClientState::Inactive);
    }

    return 0;
}