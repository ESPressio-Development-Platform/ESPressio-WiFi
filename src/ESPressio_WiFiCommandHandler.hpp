#pragma once

#include <sstream>
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

        root.Command("status").Description("Show current WiFi state").OnExecute([this](const Command::CommandContext&) {
            const auto& s = _manager->State(); std::ostringstream out;
            out << "mode=" << ModeName(s.Mode) << " ap=" << APStateName(s.AccessPoint.State)
                << " stations=" << s.AccessPoint.ConnectedStations << " client=" << ClientStateName(s.Client.State)
                << " ip=" << s.Client.Network.Address.ToString() << " scan=" << ScanStateName(s.Scan);
            return Command::CommandResult::Ok(out.str());
        });

        auto& mode = root.Command("mode").Description("Set WiFi operating mode");
        mode.Parameter("mode", Command::ParameterKind::Enumeration).OneOf({"disabled","client","ap","ap-client"});
        mode.OnExecute([this](const Command::CommandContext& c) {
            auto config = _manager->Configuration(); const auto text = c.Get<std::string>("mode");
            if (text=="disabled") config.Mode=WiFiMode::Disabled; else if(text=="client") config.Mode=WiFiMode::Client;
            else if(text=="ap") config.Mode=WiFiMode::AccessPoint; else config.Mode=WiFiMode::AccessPointClient;
            return Result(_manager->Configure(std::move(config)));
        });

        root.Command("scan").Description("Start asynchronous network scan").OnExecute([this](const Command::CommandContext&) { return Result(_manager->Scan()); });

        auto& client = root.Command("client");
        client.Command("connect").OnExecute([this](const Command::CommandContext&){ return Result(_manager->ConnectClient()); });
        client.Command("disconnect").OnExecute([this](const Command::CommandContext&){ return Result(_manager->DisconnectClient()); });
        auto& clientSsid = client.Command("ssid"); clientSsid.Parameter<std::string>("ssid"); clientSsid.OnExecute([this](const Command::CommandContext& c){ auto cfg=_manager->Configuration(); cfg.Client.SSID=c.Get<std::string>("ssid"); return Result(_manager->Configure(std::move(cfg))); });
        auto& clientPassword = client.Command("password"); clientPassword.Parameter<std::string>("password"); clientPassword.OnExecute([this](const Command::CommandContext& c){ auto cfg=_manager->Configuration(); cfg.Client.Password=c.Get<std::string>("password"); return Result(_manager->Configure(std::move(cfg))); });

        auto& ap = root.Command("ap");
        ap.Command("start").OnExecute([this](const Command::CommandContext&){ return Result(_manager->StartAccessPoint()); });
        ap.Command("stop").OnExecute([this](const Command::CommandContext&){ return Result(_manager->StopAccessPoint()); });
        auto& apSsid = ap.Command("ssid"); apSsid.Parameter<std::string>("ssid"); apSsid.OnExecute([this](const Command::CommandContext& c){ auto cfg=_manager->Configuration(); cfg.AccessPoint.SSID=c.Get<std::string>("ssid"); return Result(_manager->Configure(std::move(cfg))); });
        auto& apPassword = ap.Command("password"); apPassword.Parameter<std::string>("password"); apPassword.OnExecute([this](const Command::CommandContext& c){ auto cfg=_manager->Configuration(); cfg.AccessPoint.Password=c.Get<std::string>("password"); return Result(_manager->Configure(std::move(cfg))); });
        auto& channel = ap.Command("channel"); channel.Parameter<unsigned int>("channel").Range(1,14); channel.OnExecute([this](const Command::CommandContext& c){ auto cfg=_manager->Configuration(); cfg.AccessPoint.Channel=static_cast<uint8_t>(c.Get<unsigned int>("channel")); return Result(_manager->Configure(std::move(cfg))); });
        return true;
    }

    void Shutdown() { _registration.Reset(); _manager=nullptr; }

private:
    static Command::CommandResult Result(WiFiStatus status) { return status==WiFiStatus::Success ? Command::CommandResult::Ok("OK") : Command::CommandResult::Error(StatusName(status)); }
    static const char* StatusName(WiFiStatus v) { switch(v){case WiFiStatus::Success:return "success";case WiFiStatus::InvalidConfiguration:return "invalid configuration";case WiFiStatus::NotSupported:return "not supported";case WiFiStatus::Busy:return "busy";default:return "platform error";} }
    static const char* ModeName(WiFiMode v) { switch(v){case WiFiMode::Disabled:return "disabled";case WiFiMode::Client:return "client";case WiFiMode::AccessPoint:return "ap";default:return "ap-client";} }
    static const char* ClientStateName(ClientState v) { switch(v){case ClientState::Disabled:return "disabled";case ClientState::Idle:return "idle";case ClientState::Connecting:return "connecting";case ClientState::Connected:return "connected";case ClientState::Reconnecting:return "reconnecting";default:return "failed";} }
    static const char* APStateName(AccessPointState v) { switch(v){case AccessPointState::Disabled:return "disabled";case AccessPointState::Starting:return "starting";case AccessPointState::Active:return "active";default:return "failed";} }
    static const char* ScanStateName(ScanState v) { switch(v){case ScanState::Idle:return "idle";case ScanState::Scanning:return "scanning";case ScanState::Complete:return "complete";default:return "failed";} }
    WiFiManager* _manager=nullptr;
    Command::CommandRegistrationHandle _registration;
};

} // namespace ESPressio::WiFi
