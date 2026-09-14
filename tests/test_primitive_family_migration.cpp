#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <thread>

#include <ESPressio_Commands.hpp>
#include <ESPressio_EventTypeDescriptor.hpp>
#include <ESPressio_SerializationTraits.hpp>
#include <HostRuntime.hpp>

#include <ESPressio_WiFiCommandHandler.hpp>
#include <ESPressio_WiFiEventBridge.hpp>

using namespace ESPressio;

namespace {

template<class... T>
constexpr bool AllBounded() noexcept {
    return (Serializable::IsBoundedSerializable<T> && ...);
}

static_assert(AllBounded<
    Event::WiFiModeChangedEvent,
    Event::WiFiClientStateChangedEvent,
    Event::WiFiAccessPointStateChangedEvent,
    Event::WiFiAPUntilClientStateChangedEvent,
    Event::WiFiScanStateChangedEvent,
    Event::WiFiScanCompletedEvent,
    Event::WiFiAccessPointStationConnectedEvent,
    Event::WiFiAccessPointStationDisconnectedEvent,
    Event::WiFiClientIPAddressAcquiredEvent,
    Event::WiFiClientIPAddressLostEvent,
    Event::WiFiClientNetworkSelectionChangedEvent,
    Event::WiFiClientNetworkSelectedEvent,
    Event::WiFiClientNoKnownNetworkAvailableEvent>());

static_assert(AllBounded<
    WiFi::WiFiConfigureCommand,
    WiFi::WiFiDisableCommand,
    WiFi::WiFiConnectClientCommand,
    WiFi::WiFiDisconnectClientCommand,
    WiFi::WiFiStartAccessPointCommand,
    WiFi::WiFiStopAccessPointCommand,
    WiFi::WiFiScanCommand,
    WiFi::WiFiRetryKnownNetworksCommand,
    WiFi::WiFiUpsertClientNetworkCommand,
    WiFi::WiFiRemoveClientNetworkCommand,
    WiFi::WiFiSetClientNetworkPriorityCommand,
    WiFi::WiFiSaveConfigurationCommand,
    WiFi::WiFiLoadConfigurationCommand,
    WiFi::WiFiCommandResponse>());

class FakePlatform final : public WiFi::IWiFiPlatform {
public:
    WiFi::WiFiStatus Apply(const WiFi::WiFiConfiguration& configuration) override {
        State.Mode = configuration.Mode;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus Disable() override {
        State.Mode = WiFi::WiFiMode::Off;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus ConnectClient() override {
        ++Connects;
        State.Client.State = WiFi::ClientState::Connecting;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus DisconnectClient() override {
        ++Disconnects;
        State.Client.State = WiFi::ClientState::Disconnected;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus StartAccessPoint() override {
        ++AccessPointStarts;
        State.AccessPoint.State = WiFi::AccessPointState::Active;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus StopAccessPoint() override {
        ++AccessPointStops;
        State.AccessPoint.State = WiFi::AccessPointState::Disabled;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus StartScan() override {
        ++Scans;
        State.Scan = WiFi::ScanState::Scanning;
        ++State.Revision;
        return WiFi::WiFiStatus::Success;
    }
    WiFi::WiFiStatus Poll(
        WiFi::WiFiRuntimeState& output,
        WiFi::WiFiVector<WiFi::ScanResult>*,
        WiFi::WiFiVector<WiFi::WiFiPlatformEvent>*
    ) override {
        output = State;
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiRuntimeState State{};
    unsigned Connects = 0;
    unsigned Disconnects = 0;
    unsigned AccessPointStarts = 0;
    unsigned AccessPointStops = 0;
    unsigned Scans = 0;
};

struct ResponseHost final {
    std::atomic<std::uint64_t> Now{1000};
    std::atomic<unsigned> Wakes{0};
    std::atomic<bool> Accepting{true};

    static bool Wake(void* context, bool) noexcept {
        ++static_cast<ResponseHost*>(context)->Wakes;
        return true;
    }
    static bool Accepts(const void* context) noexcept {
        return static_cast<const ResponseHost*>(context)->Accepting.load();
    }
    static std::uint64_t Time(const void* context) noexcept {
        return static_cast<const ResponseHost*>(context)->Now.load();
    }
};

struct Caller final {
    unsigned Callbacks = 0;
    Command::CommandCallerCompletionKind Kind = Command::CommandCallerCompletionKind::ResponseTimedOut;
    WiFi::WiFiCommandOutcome Outcome = WiFi::WiFiCommandOutcome::PlatformError;

    void OnScan(const Command::CommandCompletion<WiFi::WiFiScanCommand>& completion) {
        ++Callbacks;
        Kind = completion.Kind();
        if (const auto* response = completion.ResponseValue()) Outcome = response->Outcome;
    }
};

template<class Predicate>
void Eventually(Predicate&& predicate) {
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!predicate()) {
        assert(std::chrono::steady_clock::now() < limit);
        std::this_thread::yield();
    }
}

} // namespace

int main() {
    // The complete family projections must be P3-bounded and register without
    // duplicate semantic IDs or canonical names.
    Primitive::TypeDirectory<13> eventDirectory;
    assert(Event::RegisterWiFiEventTypes(eventDirectory) == Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(eventDirectory.Initialize() == Primitive::TypeDirectoryInitializationStatus::Success);
    assert(eventDirectory.View().Size() == 13);

    Primitive::TypeDirectory<13> commandDirectory;
    assert(WiFi::RegisterWiFiCommandTypes(commandDirectory) == Primitive::TypeDirectoryRegistrationStatus::Success);
    assert(commandDirectory.Initialize() == Primitive::TypeDirectoryInitializationStatus::Success);
    assert(commandDirectory.View().Size() == 13);

    const auto eventCommon = Event::WiFiScanCompletedEvent::GetPrimitiveTypeDescriptor();
    const auto* eventDescriptor = Event::GetEventTypeDescriptor(eventCommon);
    assert(eventDescriptor != nullptr);
    assert(eventDescriptor->Tier == Event::EventTier::Serializable);
    assert(eventDescriptor->Schema != nullptr);
    assert(eventDescriptor->DynamicallyConstructible);

    const auto commandCommon = WiFi::WiFiScanCommand::GetPrimitiveTypeDescriptor();
    const auto* commandDescriptor = Command::GetCommandTypeDescriptor(commandCommon);
    assert(commandDescriptor != nullptr);
    assert(commandDescriptor->Tier == Command::CommandTier::Serializable);
    assert(commandDescriptor->RequestSchema != nullptr);
    assert(commandDescriptor->DynamicConstruction == Command::CommandDynamicConstructionMode::RequesterRequired);

    // A WiFi observer callback never creates a predecessor Event object/queue.
    // Without an initialized final Event runtime, TryDispatch fails closed and
    // the bridge records one bounded diagnostic occurrence.
    Event::WiFiEventBridge eventBridge;
    eventBridge.OnWiFiModeChanged(WiFi::WiFiMode::Off, WiFi::WiFiMode::Client);
    assert(eventBridge.UnavailableDispatches() == 1);

    HostRuntime platformRuntime;
    System::DeviceIdentifier::Storage identityBytes{};
    identityBytes[0] = 0x57;
    identityBytes[1] = 0x49;
    assert(System::RuntimeIdentity::Install({
        System::DeviceIdentifier{identityBytes},
        System::RuntimeIncarnationId{1}
    }) == System::RuntimeIdentity::InstallationStatus::Success);

    Task::TaskExecutorConfiguration routerConfiguration{};
    routerConfiguration.Execution.Name = "wifiCommandResponseRouter";
    routerConfiguration.Execution.StackSize = 4096;
    routerConfiguration.QueueDepth = 32;
    routerConfiguration.OverflowPolicy = Task::TaskQueueOverflowPolicy::Reject;
    routerConfiguration.QueueMemoryPolicy = Task::TaskMemoryPolicy::Internal;
    Command::CommandResponseRouter<32> responseRouter(routerConfiguration);

    Command::RuntimeConfiguration runtimeConfiguration{};
    runtimeConfiguration.ExecutionLane.Name = "wifiCommandLane";
    runtimeConfiguration.ExecutionLane.StackSize = 4096;
    runtimeConfiguration.ResponseRouter = responseRouter.Binding();
    Command::Runtime commandRuntime(runtimeConfiguration);

    FakePlatform wifiPlatform;
    WiFi::WiFiManager manager(wifiPlatform);
    WiFi::WiFiCommandHandler handler;
    assert(handler.Bind(commandRuntime, manager) == Command::CommandRuntimeStatus::Success);
    assert(handler.IsBound());
    assert(commandRuntime.Initialize(commandDirectory.View()) == Command::CommandRuntimeStatus::Success);
    assert(commandRuntime.Start() == Command::CommandRuntimeStatus::Success);

    const auto resources = commandRuntime.GetResourceProfile();
    assert(resources.TypeCount == 13);
    assert(resources.DestinationResponseSlots == 26);
    assert(resources.ResponseRouterCapacity == 32);

    ResponseHost responseHost;
    Threads::ThreadHostServices services{};
    services.Owner = &responseHost;
    services.WakeFunction = &ResponseHost::Wake;
    services.AcceptingFunction = &ResponseHost::Accepts;
    services.NowFunction = &ResponseHost::Time;

    Command::ResponseCapability<1> responses;
    assert(responses.Initialize(services) == Threads::ThreadStatus::Success);
    assert(responses.FinalizeInitialization() == Threads::ThreadStatus::Success);

    Caller caller;
    auto client = responses.Client(caller);
    assert(client);
    const auto submitted = client.Execute<WiFi::WiFiScanCommand, &Caller::OnScan>(
        std::chrono::milliseconds(500));
    assert(submitted.Accepted());

    Eventually([&] { return responses.ReadyCompletions() == 1; });
    responses.Service({responseHost.Now.load(), services});

    assert(caller.Callbacks == 1);
    assert(caller.Kind == Command::CommandCallerCompletionKind::Response);
    assert(caller.Outcome == WiFi::WiFiCommandOutcome::Success);
    assert(wifiPlatform.Scans == 1);

    assert(commandRuntime.Shutdown() == Command::CommandRuntimeStatus::Success);
    return 0;
}
