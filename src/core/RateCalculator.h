#pragma once

#include "core/NetworkRate.h"

#include <optional>

namespace netstat {

class RateCalculator final {
public:
    [[nodiscard]] NetworkRate AddSnapshot(NetworkSnapshot snapshot);
    void Reset() noexcept;

private:
    std::optional<NetworkSnapshot> previousSnapshot_;
};

}  // namespace netstat
