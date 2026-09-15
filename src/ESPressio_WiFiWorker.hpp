#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

#include <ESPressio_Precision.hpp>
#include <ESPressio_ThreadWith.hpp>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

struct WiFiWorkerConfiguration {
    uint32_t IterationPeriodMilliseconds = 50;
    uint32_t DesiredExecutionBudgetMilliseconds = 5;
};

/// <summary>
/// Autonomous WiFi service worker composed from the final generic Thread host
/// and the resident Precision capability.
/// </summary>
/// <remarks>
/// The worker owns no scheduler, private wake signal, or secondary task. WiFi
/// work notifications use Precision::Bump(), which publishes immediate
/// application eligibility through the Thread's one common wake path.
/// </remarks>
class WiFiWorker final : public Threads::ThreadWith<Threads::Precision<8>> {
public:
    using PrecisionCapability = Threads::Precision<8>;
    using Base = Threads::ThreadWith<PrecisionCapability>;

    // Preserve access to the root execution/stack configuration overload. The
    // WiFi runtime-tuning overload below is a distinct domain configuration.
    using Base::Configure;

    explicit WiFiWorker(
        WiFiManager& manager,
        WiFiWorkerConfiguration configuration = {}
    )
        : Base(Threads::ThreadConfiguration{}),
          _manager(manager),
          _configuration(configuration) {
        ApplyRuntimeConfiguration(configuration);
        _manager.SetWorkSignal([this]() { Bump(); });
    }

    ~WiFiWorker() override {
        // Close the borrowed callback before member teardown, then join the
        // one root Thread execution context while this concrete owner is alive.
        _manager.SetWorkSignal({});
        (void)Shutdown();
    }

    WiFiWorkerConfiguration Configuration() const {
        std::lock_guard<std::mutex> lock(_configurationMutex);
        return _configuration;
    }

    void Configure(WiFiWorkerConfiguration configuration) {
        {
            std::lock_guard<std::mutex> lock(_configurationMutex);
            _configuration = configuration;
        }
        ApplyRuntimeConfiguration(configuration);
        Bump();
    }

    WiFiStatus LastStatus() const noexcept {
        return _lastStatus.load();
    }

protected:
    Threads::ThreadWorkDisposition OnLoop() override {
        _lastStatus.store(_manager.ProcessOnce());
        return Threads::ThreadWorkDisposition::IdleReady;
    }

private:
    static constexpr uint64_t MillisecondsToNanoseconds(uint32_t milliseconds) noexcept {
        return static_cast<uint64_t>(milliseconds) * 1000000ULL;
    }

    PrecisionCapability& Precision() noexcept {
        return GetCapability<Threads::PrecisionTag>();
    }

    void Bump() {
        Precision().Bump();
    }

    void ApplyRuntimeConfiguration(const WiFiWorkerConfiguration& configuration) {
        auto& precision = Precision();
        precision.SetCadencePeriod(
            MillisecondsToNanoseconds(configuration.IterationPeriodMilliseconds)
        );
        precision.SetIterationExecutionBudget(
            MillisecondsToNanoseconds(configuration.DesiredExecutionBudgetMilliseconds)
        );
    }

    WiFiManager& _manager;
    mutable std::mutex _configurationMutex;
    WiFiWorkerConfiguration _configuration{};
    std::atomic<WiFiStatus> _lastStatus{WiFiStatus::Success};
};

} // namespace ESPressio::WiFi
