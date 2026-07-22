#pragma once

#include "core/NetworkRate.h"
#include "core/Settings.h"
#include "platform/MonotonicClock.h"

#include <optional>

namespace netstat::windows {

class WindowsNetworkSnapshotProvider final {
public:
    [[nodiscard]] std::optional<NetworkSnapshot> Capture(
        InterfaceMode mode) const noexcept;

private:
    MonotonicClock clock_;
};

}  // namespace netstat::windows
