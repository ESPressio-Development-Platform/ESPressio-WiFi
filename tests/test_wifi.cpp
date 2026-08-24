#include <cassert>
#include <vector>
#include <ESPressio_WiFi.hpp>

using namespace ESPressio::WiFi;

class FakePlatform final : public IWiFiPlatform {
public:
    WiFiStatus Apply(const WiFiConfiguration& config) override {
        configured=config; state.Mode=config.Mode; state.Revision++;
        radioState = {};
        switch (config.Mode) {
            case WiFiMode::Client:
            case WiFiMode::APUntilClient:
                radioState.Mode=WiFiRadioMode::Station;
                radioState.StationInterfaceActive=true;
                break;
            case WiFiMode::AccessPoint:
                radioState.Mode=WiFiRadioMode::AccessPoint;
                radioState.AccessPointInterfaceActive=true;
                radioState.Channel=config.AccessPoint.Channel;
                break;
            case WiFiMode::AccessPointClient:
                radioState.Mode=WiFiRadioMode::AccessPointStation;
                radioState.StationInterfaceActive=true;
                radioState.AccessPointInterfaceActive=true;
                radioState.Channel=config.AccessPoint.Channel;
                break;
            case WiFiMode::Disabled:
            case WiFiMode::Off:
                radioState.Mode=WiFiRadioMode::Off;
                break;
        }
        return WiFiStatus::Success;
    }
    WiFiStatus Disable() override { state.Mode=WiFiMode::Disabled; state.Revision++; radioState={}; return WiFiStatus::Success; }
    WiFiStatus ConnectClient() override { state.Client.State=ClientState::Connecting; state.Revision++; legacyConnects++; radioState.StationInterfaceActive=true; if(radioState.Mode==WiFiRadioMode::AccessPoint)radioState.Mode=WiFiRadioMode::AccessPointStation; else if(radioState.Mode==WiFiRadioMode::Off)radioState.Mode=WiFiRadioMode::Station; return WiFiStatus::Success; }
    WiFiStatus ConnectClient(const ClientNetworkProfile& profile) override { connectedProfiles.push_back(profile.SSID); state.Client.State=ClientState::Connecting; state.Client.SSID=profile.SSID; state.Revision++; radioState.StationInterfaceActive=true; if(radioState.Mode==WiFiRadioMode::AccessPoint)radioState.Mode=WiFiRadioMode::AccessPointStation; else if(radioState.Mode==WiFiRadioMode::Off)radioState.Mode=WiFiRadioMode::Station; return WiFiStatus::Success; }
    WiFiStatus DisconnectClient() override { state.Client.State=ClientState::Disconnected; state.Revision++; radioState.StationConnected=false; return WiFiStatus::Success; }
    WiFiStatus StartAccessPoint() override { state.AccessPoint.State=AccessPointState::Active; state.Revision++; apStarts++; radioState.AccessPointInterfaceActive=true; radioState.Channel=configured.AccessPoint.Channel; radioState.Mode=radioState.StationInterfaceActive?WiFiRadioMode::AccessPointStation:WiFiRadioMode::AccessPoint; return WiFiStatus::Success; }
    WiFiStatus StopAccessPoint() override { state.AccessPoint.State=AccessPointState::Disabled; state.Revision++; apStops++; radioState.AccessPointInterfaceActive=false; radioState.Mode=radioState.StationInterfaceActive?WiFiRadioMode::Station:WiFiRadioMode::Off; return WiFiStatus::Success; }
    WiFiStatus StartScan() override { state.Scan=ScanState::Scanning; state.Revision++; scanStarts++; radioState.Scanning=true; return WiFiStatus::Success; }
    WiFiStatus Poll(WiFiRuntimeState& output,std::vector<ScanResult>* scans,std::vector<WiFiPlatformEvent>* events) override {
        output=state;
        if (state.Client.State==ClientState::Connected) radioState.StationConnected=true;
        else if (state.Client.State==ClientState::Disconnected || state.Client.State==ClientState::Failed) radioState.StationConnected=false;
        if (deliverScan && scans) { *scans=nextScan; deliverScan=false; radioState.Scanning=false; }
        if (state.Scan==ScanState::Complete || state.Scan==ScanState::Failed) radioState.Scanning=false;
        if (!pending.empty() && events) { *events=pending; pending.clear(); }
        return WiFiStatus::Success;
    }
    WiFiRadioState GetRadioState() const override { return radioState; }
    WiFiConfiguration configured{};
    WiFiRuntimeState state{};
    WiFiRadioState radioState{};
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

class RadioObserver final : public IWiFiRadioObserver {
public:
    void OnWiFiRadioTransitionBeginning(const WiFiRadioState& before,WiFiRadioTransitionReason reason) override { beginnings++; beforeModes.push_back(before.Mode); reasons.push_back(reason); }
    void OnWiFiRadioTransitionCompleted(const WiFiRadioState&,const WiFiRadioState& after,WiFiRadioTransitionReason) override { completions++; afterModes.push_back(after.Mode); }
    void OnWiFiRadioStateChanged(const WiFiRadioState&,const WiFiRadioState& after) override { stateChanges++; last=after; }
    void OnWiFiRadioScanBeginning(const WiFiRadioState&) override { scanBeginnings++; }
    void OnWiFiRadioScanCompleted(const WiFiRadioState& after) override { scanCompletions++; last=after; }
    int beginnings=0,completions=0,stateChanges=0,scanBeginnings=0,scanCompletions=0;
    WiFiRadioState last{};
    std::vector<WiFiRadioMode> beforeModes,afterModes;
    std::vector<WiFiRadioTransitionReason> reasons;
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

        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(wifi.State().Client.State==ClientState::Connecting);

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
    }

    {
        FakePlatform platform;
        WiFiManager wifi(platform);
        Observer observer;
        auto handle=wifi.RegisterObserver(&observer); assert(handle);
        auto config=AutomaticConfig(WiFiMode::APUntilClient);
        config.APUntilClient.FallbackTimeoutMilliseconds=100;
        config.APUntilClient.RetryScanIntervalMilliseconds=50;
        assert(wifi.Configure(config)==WiFiStatus::Success);
        assert(platform.apStarts==1);
        assert(wifi.State().APUntilClient.FallbackAccessPointActive);
        ClientNetworkProfile home; home.SSID="Home"; home.Password="home-pass"; home.Priority=200;
        assert(wifi.AddOrUpdateClientNetwork(home));
        assert(platform.scanStarts>=1);
    }

    {
        uint64_t now=1000;
        FakePlatform platform;
        WiFiManager wifi(platform,[&](){ return now; });
        Observer observer;
        auto handle=wifi.RegisterObserver(&observer); assert(handle);
        auto config=AutomaticConfig(WiFiMode::APUntilClient);
        ClientNetworkProfile home; home.SSID="Home"; home.Password="home-pass"; home.Priority=200; config.Client.Networks={home};
        config.APUntilClient.FallbackTimeoutMilliseconds=100;
        config.APUntilClient.RetryScanIntervalMilliseconds=50;
        assert(wifi.Configure(config)==WiFiStatus::Success);
        assert(wifi.State().APUntilClient.State==APUntilClientState::SeekingClient);
        assert(platform.apStarts==0);
        now=1101;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.apStarts==1);
        assert(wifi.State().APUntilClient.FallbackAccessPointActive);
        const auto scansBeforeRetry=platform.scanStarts;
        now=1152;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.scanStarts>scansBeforeRetry);
        platform.nextScan={Visible("Home",-30,6)}; platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        platform.state.Client.State=ClientState::Connected; platform.state.Client.SSID="Home"; platform.state.Revision++; platform.radioState.StationConnected=true; platform.radioState.Channel=6;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(platform.apStops==1);
        assert(!wifi.State().APUntilClient.FallbackAccessPointActive);
        assert(wifi.State().APUntilClient.State==APUntilClientState::ClientConnected);
        assert(observer.apUntilClientChanges>0);
    }

    // Dedicated low-level radio lifecycle contract. This is intentionally
    // distinct from the existing application-level IWiFiObserver tests above.
    {
        FakePlatform platform;
        WiFiManager wifi(platform);
        RadioObserver radioObserver;
        auto radioHandle=wifi.RegisterRadioObserver(&radioObserver); assert(radioHandle);

        auto config=AutomaticConfig(WiFiMode::APUntilClient);
        config.Client.Networks.clear();
        config.AccessPoint.Channel=6;
        assert(wifi.Configure(config)==WiFiStatus::Success);
        assert(platform.apStarts==1);
        const auto snapshot=wifi.RadioState();
        assert(snapshot.Mode==WiFiRadioMode::AccessPointStation);
        assert(snapshot.StationInterfaceActive);
        assert(snapshot.AccessPointInterfaceActive);
        assert(snapshot.Channel==6);
        assert(radioObserver.beginnings>=2); // Configure STA, then fallback AP start.
        assert(radioObserver.completions==radioObserver.beginnings);
        assert(radioObserver.stateChanges>=1);

        const int scanBeginBefore=radioObserver.scanBeginnings;
        const int scanCompleteBefore=radioObserver.scanCompletions;
        assert(wifi.Scan()==WiFiStatus::Success);
        assert(radioObserver.scanBeginnings==scanBeginBefore+1);
        platform.nextScan={Visible("Other",-40,11)};
        platform.deliverScan=true; platform.state.Scan=ScanState::Complete; platform.state.Revision++;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(radioObserver.scanCompletions==scanCompleteBefore+1);
        assert(!wifi.RadioState().Scanning);

        platform.state.Client.State=ClientState::Connected;
        platform.radioState.StationConnected=true;
        platform.radioState.Channel=11;
        assert(wifi.ProcessOnce()==WiFiStatus::Success);
        assert(radioObserver.last.StationConnected);
        assert(radioObserver.last.Channel==11);
    }

    return 0;
}