#pragma once

#include "core/RateFormatter.h"

#include <array>
#include <cstdint>

namespace netstat {

enum class DisplayLayout : std::uint32_t {
    Stacked,
    Arrows,
    Labels,
    Compact,
    DownloadOnly,
    UploadOnly,
};

enum class InterfaceMode : std::uint32_t {
    Physical,
    AllActive,
};

enum class PlacementMode : std::uint32_t {
    Automatic,
    OnTaskbar,
    Adjacent,
    TrayOnly,
};

enum class MonitorMode : std::uint32_t {
    Primary,
    Selected,
    All,
};

struct Settings {
    static constexpr std::uint32_t kSchemaVersion = 1;
    static constexpr std::array<double, 4> kUpdateIntervals{0.5, 1.0, 2.0, 5.0};
    static constexpr double kMinimumWidthDips = 60.0;
    static constexpr double kMaximumWidthDips = 250.0;
    static constexpr double kMinimumFontSizeDips = 9.0;
    static constexpr double kMaximumFontSizeDips = 18.0;

    std::uint32_t schemaVersion{kSchemaVersion};
    double updateIntervalSeconds{1.0};
    DisplayLayout displayLayout{DisplayLayout::Stacked};
    UnitMode unitMode{UnitMode::Bytes};
    InterfaceMode interfaceMode{InterfaceMode::Physical};
    PlacementMode placementMode{PlacementMode::Automatic};
    MonitorMode monitorMode{MonitorMode::Primary};
    double customWidthDips{};
    double fontSizeDips{11.0};
    double normalizedOffset{0.5};
    bool startAtSignIn{true};

    void Validate() noexcept;
    [[nodiscard]] static Settings Defaults() noexcept;
};

}  // namespace netstat
