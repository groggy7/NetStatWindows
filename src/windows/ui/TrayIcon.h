#pragma once

#include "core/NetworkRate.h"
#include "core/Settings.h"

#include <Windows.h>

#include <optional>
#include <string>

namespace netstat::windows {

enum class TrayCommand : UINT {
    Pause = 100,
    IntervalHalfSecond,
    IntervalOneSecond,
    IntervalTwoSeconds,
    IntervalFiveSeconds,
    LayoutStacked = 120,
    LayoutArrows,
    LayoutLabels,
    LayoutCompact,
    LayoutDownloadOnly,
    LayoutUploadOnly,
    UnitsBytes = 140,
    UnitsBits,
    InterfacesPhysical = 150,
    InterfacesAllActive,
    PlacementAutomatic = 160,
    PlacementOnTaskbar,
    PlacementAdjacent,
    PlacementTrayOnly,
    Reposition = 170,
    Settings = 180,
    Reset,
    Exit,
};

class TrayIcon final {
public:
    TrayIcon() = default;
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    [[nodiscard]] bool Add(HWND owner, UINT callbackMessage);
    [[nodiscard]] bool ReAdd();
    void Remove() noexcept;
    void UpdateTooltip(const std::wstring& tooltip);

    [[nodiscard]] std::optional<TrayCommand> ShowContextMenu(
        const Settings& settings,
        const NetworkRate& rate,
        bool paused) const;

private:
    [[nodiscard]] bool AddInternal();

    HWND owner_{};
    UINT callbackMessage_{};
    HICON icon_{};
    std::wstring tooltip_{L"NetStatBar"};
    bool added_{};
};

}  // namespace netstat::windows
