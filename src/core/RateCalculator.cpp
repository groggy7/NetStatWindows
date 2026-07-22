#include "core/RateCalculator.h"

#include <cmath>
#include <limits>
#include <utility>

namespace netstat {
namespace {

[[nodiscard]] bool AddWithoutOverflow(
    const std::uint64_t value,
    std::uint64_t& total) noexcept {
    if (value > std::numeric_limits<std::uint64_t>::max() - total) {
        return false;
    }

    total += value;
    return true;
}

}  // namespace

NetworkRate RateCalculator::AddSnapshot(NetworkSnapshot snapshot) {
    NetworkRate result{};

    if (!std::isfinite(snapshot.timestampSeconds)) {
        previousSnapshot_.reset();
        return result;
    }

    if (!previousSnapshot_.has_value()) {
        previousSnapshot_ = std::move(snapshot);
        return result;
    }

    const auto& previous = *previousSnapshot_;
    const double elapsed = snapshot.timestampSeconds - previous.timestampSeconds;

    bool valid = std::isfinite(elapsed) && elapsed > 0.0 &&
                 snapshot.countersByInterface.size() ==
                     previous.countersByInterface.size();
    std::uint64_t downDelta = 0;
    std::uint64_t upDelta = 0;

    if (valid) {
        for (const auto& [id, currentCounters] : snapshot.countersByInterface) {
            const auto previousIt = previous.countersByInterface.find(id);
            if (previousIt == previous.countersByInterface.end()) {
                valid = false;
                break;
            }

            const auto& previousCounters = previousIt->second;
            if (currentCounters.receivedBytes < previousCounters.receivedBytes ||
                currentCounters.sentBytes < previousCounters.sentBytes) {
                valid = false;
                break;
            }

            const auto received =
                currentCounters.receivedBytes - previousCounters.receivedBytes;
            const auto sent = currentCounters.sentBytes - previousCounters.sentBytes;
            if (!AddWithoutOverflow(received, downDelta) ||
                !AddWithoutOverflow(sent, upDelta)) {
                valid = false;
                break;
            }
        }
    }

    previousSnapshot_ = std::move(snapshot);
    if (!valid) {
        return result;
    }

    result.downBytesPerSecond = static_cast<double>(downDelta) / elapsed;
    result.upBytesPerSecond = static_cast<double>(upDelta) / elapsed;
    return result;
}

void RateCalculator::Reset() noexcept {
    previousSnapshot_.reset();
}

}  // namespace netstat
