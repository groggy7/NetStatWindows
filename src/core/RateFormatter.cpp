#include "core/RateFormatter.h"

#include <array>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string_view>

namespace netstat {
namespace {

constexpr double kBase = 1000.0;
constexpr std::array<std::wstring_view, 4> kByteUnits{
    L"KB/s", L"MB/s", L"GB/s", L"TB/s"};
constexpr std::array<std::wstring_view, 4> kBitUnits{
    L"Kb/s", L"Mb/s", L"Gb/s", L"Tb/s"};
constexpr std::array<std::wstring_view, 4> kCompactByteUnits{
    L"K", L"M", L"G", L"T"};
constexpr std::array<std::wstring_view, 4> kCompactBitUnits{
    L"k", L"m", L"g", L"t"};

[[nodiscard]] std::wstring RoundedInteger(const double value) {
    return std::to_wstring(static_cast<long long>(std::round(value)));
}

[[nodiscard]] std::wstring FormatNumber(
    const double value,
    const std::size_t unitIndex) {
    if (value == 0.0) {
        return L"0";
    }

    if (unitIndex == 0 || value >= 10.0) {
        return RoundedInteger(value);
    }

    std::wostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(1) << value;
    std::wstring formatted = stream.str();

    if (formatted == L"10.0" ||
        (formatted.size() >= 2 && formatted.ends_with(L".0"))) {
        return RoundedInteger(value);
    }

    return formatted;
}

struct FormattedParts {
    std::wstring number;
    std::size_t unitIndex{};
};

[[nodiscard]] FormattedParts GetParts(
    const double bytesPerSecond,
    const UnitMode unitMode) {
    const double multiplier = unitMode == UnitMode::Bits ? 8.0 : 1.0;
    const double safeRate =
        std::isfinite(bytesPerSecond) ? std::max(bytesPerSecond, 0.0) : 0.0;
    double value = safeRate * multiplier / kBase;
    std::size_t unitIndex = 0;

    while (value >= 100.0 && unitIndex < kByteUnits.size() - 1) {
        value /= kBase;
        ++unitIndex;
    }

    std::wstring number;
    while (true) {
        number = FormatNumber(value, unitIndex);
        if (number != L"100" || unitIndex == kByteUnits.size() - 1) {
            break;
        }

        value /= kBase;
        ++unitIndex;
    }

    return {.number = std::move(number), .unitIndex = unitIndex};
}

}  // namespace

std::wstring RateFormatter::Format(
    const double bytesPerSecond,
    const UnitMode unitMode) {
    auto parts = GetParts(bytesPerSecond, unitMode);
    const auto& units = unitMode == UnitMode::Bits ? kBitUnits : kByteUnits;

    if (parts.number.size() < 3) {
        parts.number.insert(0, 3 - parts.number.size(), L' ');
    }

    return parts.number + L" " + std::wstring(units[parts.unitIndex]);
}

std::wstring RateFormatter::FormatCompact(
    const double bytesPerSecond,
    const UnitMode unitMode) {
    const auto parts = GetParts(bytesPerSecond, unitMode);
    const auto& units =
        unitMode == UnitMode::Bits ? kCompactBitUnits : kCompactByteUnits;
    return parts.number + std::wstring(units[parts.unitIndex]);
}

}  // namespace netstat
