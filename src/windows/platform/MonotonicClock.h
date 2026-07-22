#pragma once

#include <optional>

namespace netstat::windows {

class MonotonicClock final {
public:
    MonotonicClock() noexcept;

    [[nodiscard]] std::optional<double> NowSeconds() const noexcept;
    [[nodiscard]] bool IsAvailable() const noexcept;

private:
    long long frequency_{};
};

}  // namespace netstat::windows
