#pragma once

#include <string>

namespace netstat {

enum class UnitMode {
    Bytes,
    Bits,
};

class RateFormatter final {
public:
    [[nodiscard]] static std::wstring Format(
        double bytesPerSecond,
        UnitMode unitMode);

    [[nodiscard]] static std::wstring FormatCompact(
        double bytesPerSecond,
        UnitMode unitMode);
};

}  // namespace netstat
