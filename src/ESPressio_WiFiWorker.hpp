#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

#include <ESPressio_PrecisionThread.hpp>
#include <ESPressio_PrecisionThreadTraits.hpp>
#include <ESPressio_Time.hpp>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

struct WiFiWorkerConfiguration {
    uint32_t IterationPeriodMilliseconds = 50;
    uint32_t DesiredExecutionBudgetMilliseconds = 5;
};

class WiFiWorker final
    : public Threads::PrecisionThread<
          Units::NanoSeconds<uint64_t>,
          Threads::PrecisionThreadTraits<Units::NanoSeconds<uint64_t>>
      > {
public:
    using Time = Units::NanoSeconds<uint64_t>;
    using Base = Threads::PrecisionThread<Time, Threads::PrecisionThreadTraits<Time>>;

    explicit WiFiWorker(
        WiFiManager& manager,
        WiFiWorkerConfiguration configuration = {}
    ) : _manager(manager), _configuration(configuration) {
        ApplyRuntimeConfiguration(configuration);
        _manager.SetWorkSignal([this]() { this->Bump(); });
    }

    ~WiFiWorker() override {
        _manager.SetWorkSignal({});
        Shutdown();
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
    void Iterate(
        Time,
        Time,
        Threads::SkippedIterationCount
    ) override {
        _lastStatus.store(_manager.ProcessOnce());
    }

private:
    void ApplyRuntimeConfiguration(
        const WiFiWorkerConfiguration& configuration
    ) {
        const auto period = Units::MilliSeconds<uint32_t>(
            configuration.IterationPeriodMilliseconds
        );
        const auto desiredExecutionBudget = Units::MilliSeconds<uint32_t>(
            configuration.DesiredExecutionBudgetMilliseconds
        );

        SetIterationPeriod(period);
        SetDesiredIterationPeriod(desiredExecutionBudget);
    }

    WiFiManager& _manager;
    mutable std::mutex _configurationMutex;
    WiFiWorkerConfiguration _configuration{};
    std::atomic<WiFiStatus> _lastStatus{WiFiStatus::Success};
};

} // namespace ESPressio::WiFi
