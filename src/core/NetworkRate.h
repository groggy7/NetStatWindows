#pragma once

#include <cstdint>
#include <unordered_map>

namespace netstat {

using InterfaceId = std::uint64_t;

struct InterfaceCounters {
    std::uint64_t receivedBytes{};
    std::uint64_t sentBytes{};

    [[nodiscard]] bool operator==(const InterfaceCounters&) const = default;
};

struct NetworkSnapshot {
    std::unordered_map<InterfaceId, InterfaceCounters> countersByInterface;
    double timestampSeconds{};
};

struct NetworkRate {
    double downBytesPerSecond{};
    double upBytesPerSecond{};

    [[nodiscard]] bool operator==(const NetworkRate&) const = default;
};

}  // namespace netstat
