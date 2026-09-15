#include <ESPressio_WiFiWorker.hpp>
#include <ESPressio_SystemPlatformClock.hpp>
#include <HostRuntime.hpp>

#include <atomic>
#include <cassert>
#include <chrono>
#include <thread>
#include <type_traits>

using namespace ESPressio;

namespace {

class ManualClock final : public System::Clock::IMonotonicClock {
public:
    std::atomic<std::uint64_t> Value{0};

    std::uint64_t NowNanoseconds() const noexcept override {
        return Value.load(std::memory_order_acquire);
    }

    std::uint64_t ResolutionNanoseconds() const noexcept override {
        return 1;
    }

    bool IsInterruptSafe() const noexcept override {
        return true;
    }
};

class FakePlatform final : public WiFi::IWiFiPlatform {
public:
    std::atomic<unsigned> Polls{0};
    WiFi::WiFiRuntimeState State{};

    WiFi::WiFiStatus Apply(const WiFi::WiFiConfiguration&) override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus Disable() override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus ConnectClient() override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus DisconnectClient() override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus StartAccessPoint() override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus StopAccessPoint() override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus StartScan() override {
        return WiFi::WiFiStatus::Success;
    }

    WiFi::WiFiStatus Poll(
        WiFi::WiFiRuntimeState& output,
        WiFi::WiFiVector<WiFi::ScanResult>*,
        WiFi::WiFiVector<WiFi::WiFiPlatformEvent>*
    ) override {
        output = State;
        Polls.fetch_add(1, std::memory_order_release);
        return WiFi::WiFiStatus::Success;
    }
};

template<class Predicate>
void Await(Predicate&& predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!predicate()) {
        assert(std::chrono::steady_clock::now() < deadline);
        std::this_thread::yield();
    }
}

} // namespace

int main() {
    static_assert(std::is_base_of_v<Threads::Thread, WiFi::WiFiWorker>);
    static_assert(WiFi::WiFiWorker::HasCapability<Threads::PrecisionTag>());

    HostRuntime runtime;
    ManualClock clock;
    System::Clock::SetMonotonicClock(&clock);

    FakePlatform platform;
    WiFi::WiFiManager manager(platform);
    WiFi::WiFiWorker worker(manager);

    const auto profile = Threads::GetThreadResourceProfile(worker);
    assert(profile.ExecutionContexts == 1);
    assert(profile.CommonWorkSignals == 1);
    assert(profile.ConfiguredStackBytes == 4096);

    assert(worker.Initialize() == Threads::ThreadStatus::Success);
    assert(worker.Start() == Threads::ThreadStatus::Success);

    // Precision activation makes the first application iteration immediately
    // eligible; WiFi therefore services once without application polling.
    Await([&] { return platform.Polls.load(std::memory_order_acquire) >= 1; });
    assert(worker.LastStatus() == WiFi::WiFiStatus::Success);

    const auto beforeBump = platform.Polls.load(std::memory_order_acquire);
    worker.Configure({25, 7});
    const auto configured = worker.Configuration();
    assert(configured.IterationPeriodMilliseconds == 25);
    assert(configured.DesiredExecutionBudgetMilliseconds == 7);

    // Runtime tuning ends with Precision::Bump(); the manager/worker work path
    // therefore uses the root common Wake rather than a private scheduler signal.
    Await([&] {
        return platform.Polls.load(std::memory_order_acquire) > beforeBump;
    });

    assert(worker.Shutdown() == Threads::ThreadStatus::Success);
    System::Clock::ResetMonotonicClock();
    return 0;
}
