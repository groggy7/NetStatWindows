#include "core/Settings.h"

#include <algorithm>
#include <cmath>

namespace netstat {
namespace {

template <typename Enum>
[[nodiscard]] bool EnumInRange(
    const Enum value,
    const Enum first,
    const Enum last) noexcept {
    return value >= first && value <= last;
}

}  // namespace

Settings Settings::Defaults() noexcept {
    return {};
}

void Settings::Validate() noexcept {
    const Settings defaults = Defaults();
    schemaVersion = kSchemaVersion;

    const bool intervalIsValid = std::isfinite(updateIntervalSeconds) &&
        std::ranges::find(kUpdateIntervals, updateIntervalSeconds) !=
            kUpdateIntervals.end();
    if (!intervalIsValid) {
        updateIntervalSeconds = defaults.updateIntervalSeconds;
    }

    if (!EnumInRange(
            displayLayout,
            DisplayLayout::Stacked,
            DisplayLayout::UploadOnly)) {
        displayLayout = defaults.displayLayout;
    }
    if (!EnumInRange(unitMode, UnitMode::Bytes, UnitMode::Bits)) {
        unitMode = defaults.unitMode;
    }
    if (!EnumInRange(
            interfaceMode,
            InterfaceMode::Physical,
            InterfaceMode::AllActive)) {
        interfaceMode = defaults.interfaceMode;
    }
    if (!EnumInRange(
            placementMode,
            PlacementMode::Automatic,
            PlacementMode::TrayOnly)) {
        placementMode = defaults.placementMode;
    }
    if (!EnumInRange(
            monitorMode,
            MonitorMode::Primary,
            MonitorMode::All)) {
        monitorMode = defaults.monitorMode;
    }

    const bool widthIsValid = std::isfinite(customWidthDips) &&
        (customWidthDips == 0.0 ||
         (customWidthDips >= kMinimumWidthDips &&
          customWidthDips <= kMaximumWidthDips));
    if (!widthIsValid) {
        customWidthDips = defaults.customWidthDips;
    }

    if (!std::isfinite(fontSizeDips) ||
        fontSizeDips < kMinimumFontSizeDips ||
        fontSizeDips > kMaximumFontSizeDips) {
        fontSizeDips = defaults.fontSizeDips;
    }

    if (!std::isfinite(normalizedOffset)) {
        normalizedOffset = defaults.normalizedOffset;
    } else {
        normalizedOffset = std::clamp(normalizedOffset, 0.0, 1.0);
    }
}

}  // namespace netstat
