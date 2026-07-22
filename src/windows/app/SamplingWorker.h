#pragma once

#include "core/NetworkRate.h"
#include "core/Settings.h"
#include "platform/WindowsNetworkSnapshotProvider.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace netstat::windows {

class SamplingWorker final {
public:
    using Callback = std::function<void(NetworkRate)>;

    explicit SamplingWorker(Callback callback);
    ~SamplingWorker();

    SamplingWorker(const SamplingWorker&) = delete;
    SamplingWorker& operator=(const SamplingWorker&) = delete;

    void Start(InterfaceMode mode, double intervalSeconds);
    void Reconfigure(InterfaceMode mode, double intervalSeconds);
    void ResetBaseline();
    void Stop() noexcept;

private:
    void Run(std::stop_token stopToken);

    Callback callback_;
    WindowsNetworkSnapshotProvider snapshotProvider_;
    std::mutex mutex_;
    std::condition_variable_any condition_;
    std::jthread thread_;
    InterfaceMode interfaceMode_{InterfaceMode::Physical};
    double intervalSeconds_{1.0};
    std::uint64_t configurationVersion_{};
};

}  // namespace netstat::windows
