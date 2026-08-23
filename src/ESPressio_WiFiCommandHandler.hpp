#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <ESPressio_Command.hpp>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

class WiFiCommandHandler {
public:
    bool Initialize(Command::CommandRegistry& registry, WiFiManager& manager) {
        if (_registration.Active()) return true;
        _manager = &manager;
        _registration = registry.RegisterCommand("wifi");
        if (!_registration.Active()) return false;

        auto& root = registry.Command("wifi").Description("ESPressio WiFi control and diagnostics");
        root.Command("status").Description("Show composite AP/client/scan WiFi state")
            .OnExecute([this](const Command::CommandContext&) { return Status(); });

        auto& mode = root.Command("mode").Description("Set WiFi operating mode");
        mode.Parameter("mode", Command::ParameterKind::Enumeration).OneOf({"disabled","client","ap","ap-client"});
        mode.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration();
            const auto text = context.Get<std::string>("mode");
            if (text == "disabled") config.Mode = WiFiMode::Disabled;
            else if (text == "client") config.Mode = WiFiMode::Client;
            else if (text == "ap") config.Mode = WiFiMode::AccessPoint;
            else config.Mode = WiFiMode::AccessPointClient;
            return Result(_manager->Configure(std::move(config)));
        });

        auto& scan = root.Command("scan").Description("WiFi network scanning");
        scan.OnExecute([this](const Command::CommandContext&) { return Result(_manager->Scan()); });
        scan.Command("results").Description("Show the most recent completed scan")
            .OnExecute([this](const Command::CommandContext&) { return ScanResults(); });

        ConfigureClientCommands(root.Command("client").Description("Station/client controls"));
        ConfigureAccessPointCommands(root.Command("ap").Description("Access Point controls"));
        ConfigureConfigurationCommands(root.Command("config").Description("Safe WiFi configuration controls"));
        return true;
    }

    void Shutdown() { _registration.Reset(); _manager = nullptr; }

private:
    void ConfigureClientCommands(Command::CommandNode& client) {
        client.Command("status").OnExecute([this](const Command::CommandContext&) {
            const auto state = _manager->State().Client;
            std::ostringstream out;
            out << "state=" << ClientStateName(state.State)
                << " ssid=" << state.SSID
                << " rssi=" << state.RSSI
                << " channel=" << static_cast<unsigned>(state.Channel)
                << " ip=" << state.Network.Address.ToString()
                << " reconnect-attempt=" << state.ReconnectAttempt
                << " selection=" << SelectionStateName(state.Selection.State)
                << " candidates=" << state.Selection.EligibleCandidateCount;
            if (!state.Selection.SelectedSSID.empty()) {
                out << " selected=" << state.Selection.SelectedSSID
                    << " priority=" << state.Selection.SelectedPriority;
            }
            return Command::CommandResult::Ok(out.str());
        });
        client.Command("connect").OnExecute([this](const Command::CommandContext&) { return Result(_manager->ConnectClient()); });
        client.Command("disconnect").OnExecute([this](const Command::CommandContext&) { return Result(_manager->DisconnectClient()); });

        auto& autoSelect = client.Command("auto-select");
        autoSelect.Parameter<bool>("enabled");
        autoSelect.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration();
            config.Client.Selection.AutomaticSelection = context.Get<bool>("enabled");
            return Result(_manager->Configure(std::move(config)));
        });

        auto& networks = client.Command("networks").Description("Manage remembered client networks");
        networks.Command("list").OnExecute([this](const Command::CommandContext&) { return RememberedNetworks(); });

        auto& add = networks.Command("add").Description("Add or update a remembered network; password is never returned");
        add.Parameter<std::string>("ssid");
        add.Parameter<std::string>("password");
        add.Parameter<unsigned int>("priority").Range(0,65535);
        add.OnExecute([this](const Command::CommandContext& context) {
            ClientNetworkProfile profile;
            profile.SSID = context.Get<std::string>("ssid");
            profile.Password = context.Get<std::string>("password");
            profile.Priority = static_cast<uint16_t>(context.Get<unsigned int>("priority"));
            if (!_manager->AddOrUpdateClientNetwork(std::move(profile))) return Command::CommandResult::Error("Invalid remembered network");
            return Command::CommandResult::Ok("OK");
        });

        auto& remove = networks.Command("remove");
        remove.Parameter<std::string>("ssid");
        remove.OnExecute([this](const Command::CommandContext& context) {
            return _manager->RemoveClientNetwork(context.Get<std::string>("ssid"))
                ? Command::CommandResult::Ok("OK") : Command::CommandResult::Error("Remembered network not found");
        });

        auto& priority = networks.Command("priority");
        priority.Parameter<std::string>("ssid");
        priority.Parameter<unsigned int>("priority").Range(0,65535);
        priority.OnExecute([this](const Command::CommandContext& context) {
            return _manager->SetClientNetworkPriority(
                context.Get<std::string>("ssid"), static_cast<uint16_t>(context.Get<unsigned int>("priority")))
                ? Command::CommandResult::Ok("OK") : Command::CommandResult::Error("Remembered network not found");
        });

        // Legacy single-network controls remain for 0.1.x source compatibility.
        auto& ssid = client.Command("ssid");
        ssid.Parameter<std::string>("ssid");
        ssid.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration();
            config.Client.SSID = context.Get<std::string>("ssid");
            config.Client.Enabled = true;
            return Result(_manager->Configure(std::move(config)));
        });

        auto& password = client.Command("password");
        password.Description("Set legacy client password; passwords are never returned by commands");
        password.Parameter<std::string>("password");
        password.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration();
            config.Client.Password = context.Get<std::string>("password");
            return Result(_manager->Configure(std::move(config)));
        });

        auto& addressing = client.Command("addressing");
        addressing.Parameter("mode", Command::ParameterKind::Enumeration).OneOf({"dhcp","static"});
        addressing.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration();
            config.Client.Addressing = context.Get<std::string>("mode") == "dhcp" ? AddressMode::DHCP : AddressMode::Static;
            return Result(_manager->Configure(std::move(config)));
        });

        AddIPv4Setter(client, "ip", [](WiFiConfiguration& c, const IPv4Address& v){ c.Client.StaticNetwork.Address = v; });
        AddIPv4Setter(client, "gateway", [](WiFiConfiguration& c, const IPv4Address& v){ c.Client.StaticNetwork.Gateway = v; });
        AddIPv4Setter(client, "subnet", [](WiFiConfiguration& c, const IPv4Address& v){ c.Client.StaticNetwork.SubnetMask = v; });
        AddIPv4Setter(client, "dns1", [](WiFiConfiguration& c, const IPv4Address& v){ c.Client.StaticNetwork.PrimaryDNS = v; });
        AddIPv4Setter(client, "dns2", [](WiFiConfiguration& c, const IPv4Address& v){ c.Client.StaticNetwork.SecondaryDNS = v; });
    }

    void ConfigureAccessPointCommands(Command::CommandNode& ap) {
        ap.Command("status").OnExecute([this](const Command::CommandContext&) {
            const auto state = _manager->State().AccessPoint;
            std::ostringstream out;
            out << "state=" << APStateName(state.State) << " ssid=" << state.SSID
                << " channel=" << static_cast<unsigned>(state.Channel) << " stations=" << state.ConnectedStations
                << " ip=" << state.Network.Address.ToString();
            return Command::CommandResult::Ok(out.str());
        });
        ap.Command("start").OnExecute([this](const Command::CommandContext&) { return Result(_manager->StartAccessPoint()); });
        ap.Command("stop").OnExecute([this](const Command::CommandContext&) { return Result(_manager->StopAccessPoint()); });

        auto& ssid = ap.Command("ssid");
        ssid.Parameter<std::string>("ssid");
        ssid.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration(); config.AccessPoint.SSID = context.Get<std::string>("ssid"); config.AccessPoint.Enabled = true;
            return Result(_manager->Configure(std::move(config)));
        });

        auto& password = ap.Command("password");
        password.Description("Set AP password; passwords are never returned by commands");
        password.Parameter<std::string>("password");
        password.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration(); config.AccessPoint.Password = context.Get<std::string>("password");
            return Result(_manager->Configure(std::move(config)));
        });

        auto& channel = ap.Command("channel");
        channel.Parameter<unsigned int>("channel").Range(1,14);
        channel.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration(); config.AccessPoint.Channel = static_cast<uint8_t>(context.Get<unsigned int>("channel"));
            return Result(_manager->Configure(std::move(config)));
        });

        auto& dhcp = ap.Command("dhcp");
        auto& enabled = dhcp.Command("enabled");
        enabled.Parameter<bool>("enabled");
        enabled.OnExecute([this](const Command::CommandContext& context) {
            auto config = _manager->Configuration(); config.AccessPoint.DHCP.Enabled = context.Get<bool>("enabled");
            return Result(_manager->Configure(std::move(config)));
        });
        AddIPv4Setter(dhcp, "lease-start", [](WiFiConfiguration& c, const IPv4Address& v){ c.AccessPoint.DHCP.LeaseStart = v; });
        AddIPv4Setter(dhcp, "lease-end", [](WiFiConfiguration& c, const IPv4Address& v){ c.AccessPoint.DHCP.LeaseEnd = v; });
    }

    void ConfigureConfigurationCommands(Command::CommandNode& config) {
        config.Command("show").Description("Show configuration with credentials always redacted")
            .OnExecute([this](const Command::CommandContext&) { return ShowConfiguration(); });
        config.Command("save").OnExecute([this](const Command::CommandContext&) { return StoreResult(_manager->SaveConfiguration()); });
        config.Command("load").OnExecute([this](const Command::CommandContext&) { return StoreResult(_manager->LoadConfiguration(true)); });
        config.Command("reset").OnExecute([this](const Command::CommandContext&) { return Result(_manager->Configure(WiFiConfiguration{})); });

        auto& hostname = config.Command("hostname"); hostname.Parameter<std::string>("hostname");
        hostname.OnExecute([this](const Command::CommandContext& context) {
            auto value = _manager->Configuration(); value.Hostname = context.Get<std::string>("hostname"); return Result(_manager->Configure(std::move(value)));
        });
        auto& tx = config.Command("tx-power"); tx.Parameter<int>("dbm").Range(2,20);
        tx.OnExecute([this](const Command::CommandContext& context) {
            auto value = _manager->Configuration(); value.TxPowerDbm = static_cast<int8_t>(context.Get<int>("dbm")); return Result(_manager->Configure(std::move(value)));
        });
        auto& powerSave = config.Command("power-save"); powerSave.Parameter<bool>("enabled");
        powerSave.OnExecute([this](const Command::CommandContext& context) {
            auto value = _manager->Configuration(); value.PowerSave = context.Get<bool>("enabled"); return Result(_manager->Configure(std::move(value)));
        });
    }

    template<typename Setter>
    void AddIPv4Setter(Command::CommandNode& parent, const char* name, Setter setter) {
        auto& command = parent.Command(name); command.Parameter<std::string>("address");
        command.OnExecute([this, setter](const Command::CommandContext& context) {
            IPv4Address address;
            if (!ParseIPv4(context.Get<std::string>("address"), address)) return Command::CommandResult::Error("Invalid IPv4 address");
            auto config = _manager->Configuration(); setter(config, address); return Result(_manager->Configure(std::move(config)));
        });
    }

    Command::CommandResult Status() const {
        const auto state = _manager->State();
        std::ostringstream out;
        out << "mode=" << ModeName(state.Mode) << " ap=" << APStateName(state.AccessPoint.State)
            << " stations=" << state.AccessPoint.ConnectedStations << " client=" << ClientStateName(state.Client.State)
            << " ip=" << state.Client.Network.Address.ToString() << " scan=" << ScanStateName(state.Scan)
            << " selection=" << SelectionStateName(state.Client.Selection.State);
        return Command::CommandResult::Ok(out.str());
    }

    Command::CommandResult ScanResults() const {
        const auto results = _manager->LastScanResults();
        std::ostringstream out; out << "networks=" << results.size();
        for (std::size_t i = 0; i < results.size(); ++i) {
            out << "\n[" << i << "] ssid=" << results[i].SSID << " rssi=" << results[i].RSSI
                << " channel=" << static_cast<unsigned>(results[i].Channel) << " security=" << SecurityName(results[i].Security);
        }
        return Command::CommandResult::Ok(out.str());
    }

    Command::CommandResult RememberedNetworks() const {
        const auto config = _manager->Configuration();
        std::vector<ClientNetworkProfile> profiles = config.Client.Networks;
        std::sort(profiles.begin(), profiles.end(), [](const ClientNetworkProfile& a, const ClientNetworkProfile& b) {
            if (a.Priority != b.Priority) return a.Priority > b.Priority;
            return a.SSID < b.SSID;
        });
        std::ostringstream out; out << "remembered-networks=" << profiles.size();
        for (const auto& profile : profiles) {
            out << "\nssid=" << profile.SSID << " priority=" << profile.Priority
                << " enabled=" << (profile.Enabled ? "true" : "false") << " password=<redacted>"
                << " addressing=" << (profile.Addressing == AddressMode::DHCP ? "dhcp" : "static");
        }
        return Command::CommandResult::Ok(out.str());
    }

    Command::CommandResult ShowConfiguration() const {
        const auto c = _manager->Configuration();
        std::ostringstream out;
        out << "mode=" << ModeName(c.Mode) << " hostname=" << c.Hostname
            << " tx-power=" << static_cast<int>(c.TxPowerDbm) << " power-save=" << (c.PowerSave ? "true" : "false")
            << "\nap.ssid=" << c.AccessPoint.SSID << " ap.password=<redacted> ap.channel=" << static_cast<unsigned>(c.AccessPoint.Channel)
            << " ap.dhcp=" << (c.AccessPoint.DHCP.Enabled ? "true" : "false")
            << "\nclient.automatic-selection=" << (c.Client.Selection.AutomaticSelection ? "true" : "false")
            << " client.remembered-networks=" << c.Client.Networks.size()
            << "\nclient.legacy.ssid=" << c.Client.SSID << " client.legacy.password=<redacted>"
            << " client.legacy.addressing=" << (c.Client.Addressing == AddressMode::DHCP ? "dhcp" : "static");
        return Command::CommandResult::Ok(out.str());
    }

    static Command::CommandResult Result(WiFiStatus status) {
        return status == WiFiStatus::Success ? Command::CommandResult::Ok("OK") : Command::CommandResult::Error(StatusName(status));
    }
    static Command::CommandResult StoreResult(const WiFiConfigurationStoreResult& result) {
        return result.Success() ? Command::CommandResult::Ok("OK") : Command::CommandResult::Error(result.Message.empty() ? "WiFi configuration persistence failed" : result.Message);
    }
    static bool ParseIPv4(const std::string& text, IPv4Address& output) {
        unsigned a=0,b=0,c=0,d=0; char tail=0;
        if (std::sscanf(text.c_str(), "%u.%u.%u.%u%c", &a,&b,&c,&d,&tail) != 4 || a>255 || b>255 || c>255 || d>255) return false;
        output = IPv4Address(static_cast<uint8_t>(a),static_cast<uint8_t>(b),static_cast<uint8_t>(c),static_cast<uint8_t>(d)); return true;
    }
    static const char* StatusName(WiFiStatus v){switch(v){case WiFiStatus::Success:return"success";case WiFiStatus::InvalidConfiguration:return"invalid configuration";case WiFiStatus::NotSupported:return"not supported";case WiFiStatus::Busy:return"busy";default:return"platform error";}}
    static const char* ModeName(WiFiMode v){switch(v){case WiFiMode::Disabled:return"disabled";case WiFiMode::Client:return"client";case WiFiMode::AccessPoint:return"ap";default:return"ap-client";}}
    static const char* ClientStateName(ClientState v){switch(v){case ClientState::Disabled:return"disabled";case ClientState::Idle:return"idle";case ClientState::Connecting:return"connecting";case ClientState::Connected:return"connected";case ClientState::Reconnecting:return"reconnecting";case ClientState::Disconnecting:return"disconnecting";case ClientState::Disconnected:return"disconnected";default:return"failed";}}
    static const char* APStateName(AccessPointState v){switch(v){case AccessPointState::Disabled:return"disabled";case AccessPointState::Starting:return"starting";case AccessPointState::Active:return"active";default:return"failed";}}
    static const char* ScanStateName(ScanState v){switch(v){case ScanState::Idle:return"idle";case ScanState::Scanning:return"scanning";case ScanState::Complete:return"complete";default:return"failed";}}
    static const char* SecurityName(NetworkSecurity v){switch(v){case NetworkSecurity::Open:return"open";case NetworkSecurity::WEP:return"wep";case NetworkSecurity::WPA:return"wpa";case NetworkSecurity::WPA2:return"wpa2";case NetworkSecurity::WPA_WPA2:return"wpa-wpa2";case NetworkSecurity::WPA3:return"wpa3";case NetworkSecurity::WPA2_WPA3:return"wpa2-wpa3";default:return"unknown";}}
    static const char* SelectionStateName(ClientNetworkSelectionState v){switch(v){case ClientNetworkSelectionState::Idle:return"idle";case ClientNetworkSelectionState::Scanning:return"scanning";case ClientNetworkSelectionState::Selecting:return"selecting";case ClientNetworkSelectionState::Connecting:return"connecting";case ClientNetworkSelectionState::Connected:return"connected";case ClientNetworkSelectionState::NoKnownNetworkAvailable:return"no-known-network";default:return"exhausted";}}

    WiFiManager* _manager = nullptr;
    Command::CommandRegistrationHandle _registration;
};

} // namespace ESPressio::WiFi
