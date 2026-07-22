#include "platform/MonotonicClock.h"

#include <Windows.h>

namespace netstat::windows {

MonotonicClock::MonotonicClock() noexcept {
    LARGE_INTEGER frequency{};
    if (::QueryPerformanceFrequency(&frequency) != FALSE &&
        frequency.QuadPart > 0) {
        frequency_ = frequency.QuadPart;
    }
}

std::optional<double> MonotonicClock::NowSeconds() const noexcept {
    if (!IsAvailable()) {
        return std::nullopt;
    }

    LARGE_INTEGER counter{};
    if (::QueryPerformanceCounter(&counter) == FALSE) {
        return std::nullopt;
    }

    return static_cast<double>(counter.QuadPart) /
        static_cast<double>(frequency_);
}

bool MonotonicClock::IsAvailable() const noexcept {
    return frequency_ > 0;
}

}  // namespace netstat::windows
