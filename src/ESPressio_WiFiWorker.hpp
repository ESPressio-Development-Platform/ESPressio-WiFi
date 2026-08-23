#pragma once

#include <cstdint>

#include <ESPressio_PrecisionThread.hpp>
#include <ESPressio_PrecisionThreadTraits.hpp>
#include <ESPressio_Time.hpp>
#include "ESPressio_WiFi.hpp"

namespace ESPressio::WiFi {

struct WiFiWorkerConfiguration {
    uint32_t IterationPeriodMilliseconds = 50;
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
        ApplyRuntimeConfiguration();
        _manager.SetWorkSignal([this]() { this->Bump(); });
    }

    ~WiFiWorker() override {
        _manager.SetWorkSignal({});
        Shutdown();
    }

    const WiFiWorkerConfiguration& Configuration() const noexcept {
        return _configuration;
    }

    void Configure(WiFiWorkerConfiguration configuration) {
        _configuration = configuration;
        ApplyRuntimeConfiguration();
        Bump();
    }

    WiFiStatus LastStatus() const noexcept { return _lastStatus; }

protected:
    void Iterate(
        Time,
        Time,
        Threads::SkippedIterationCount
    ) override {
        _lastStatus = _manager.ProcessOnce();
    }

private:
    void ApplyRuntimeConfiguration() {
        const auto period = Units::MilliSeconds<uint32_t>(
            _configuration.IterationPeriodMilliseconds
        );
        SetIterationPeriod(period);
        // PrecisionThread constrains desired period to at least the scheduling
        // period. Keep both aligned and let Threads own overrun diagnostics.
        SetDesiredIterationPeriod(period);
    }

    WiFiManager& _manager;
    WiFiWorkerConfiguration _configuration{};
    WiFiStatus _lastStatus = WiFiStatus::Success;
};

} // namespace ESPressio::WiFi
