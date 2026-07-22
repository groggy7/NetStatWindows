#include "app/SamplingWorker.h"

#include "core/RateCalculator.h"

#include <chrono>
#include <utility>

namespace netstat::windows {

SamplingWorker::SamplingWorker(Callback callback)
    : callback_(std::move(callback)) {}

SamplingWorker::~SamplingWorker() {
    Stop();
}

void SamplingWorker::Start(
    const InterfaceMode mode,
    const double intervalSeconds) {
    Stop();
    {
        const std::scoped_lock lock(mutex_);
        interfaceMode_ = mode;
        intervalSeconds_ = intervalSeconds;
        ++configurationVersion_;
    }
    thread_ = std::jthread([this](const std::stop_token stopToken) {
        Run(stopToken);
    });
}

void SamplingWorker::Reconfigure(
    const InterfaceMode mode,
    const double intervalSeconds) {
    {
        const std::scoped_lock lock(mutex_);
        interfaceMode_ = mode;
        intervalSeconds_ = intervalSeconds;
        ++configurationVersion_;
    }
    condition_.notify_all();
}

void SamplingWorker::ResetBaseline() {
    {
        const std::scoped_lock lock(mutex_);
        ++configurationVersion_;
    }
    condition_.notify_all();
}

void SamplingWorker::Stop() noexcept {
    if (thread_.joinable()) {
        thread_.request_stop();
        condition_.notify_all();
        thread_.join();
    }
}

void SamplingWorker::Run(const std::stop_token stopToken) {
    RateCalculator calculator;
    std::uint64_t observedVersion = 0;

    while (!stopToken.stop_requested()) {
        InterfaceMode mode;
        double interval;
        {
            const std::scoped_lock lock(mutex_);
            mode = interfaceMode_;
            interval = intervalSeconds_;
            observedVersion = configurationVersion_;
        }

        calculator.Reset();
        const auto baseline = snapshotProvider_.Capture(mode);
        if (baseline.has_value()) {
            const auto ignoredRate = calculator.AddSnapshot(*baseline);
            static_cast<void>(ignoredRate);
        }
        callback_(NetworkRate{});

        while (!stopToken.stop_requested()) {
            std::unique_lock lock(mutex_);
            const auto duration = std::chrono::duration<double>(interval);
            const bool interrupted = condition_.wait_for(
                lock,
                stopToken,
                duration,
                [this, observedVersion] {
                    return configurationVersion_ != observedVersion;
                });
            if (stopToken.stop_requested()) {
                return;
            }
            if (interrupted) {
                break;
            }

            mode = interfaceMode_;
            lock.unlock();

            const auto snapshot = snapshotProvider_.Capture(mode);
            if (!snapshot.has_value()) {
                calculator.Reset();
                callback_(NetworkRate{});
                continue;
            }
            callback_(calculator.AddSnapshot(*snapshot));
        }
    }
}

}  // namespace netstat::windows
