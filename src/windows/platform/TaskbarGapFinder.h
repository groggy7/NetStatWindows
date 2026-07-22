#pragma once

#include "platform/TaskbarLocator.h"

#include <Windows.h>

#include <optional>

namespace netstat::windows {

class TaskbarGapFinder final {
public:
    [[nodiscard]] std::optional<RECT> FindSafeGap(
        const TaskbarInfo& taskbar,
        long readoutWidth,
        long readoutHeight,
        double preferredNormalizedOffset) const noexcept;

    [[nodiscard]] static RECT PlaceAtOffset(
        const TaskbarInfo& taskbar,
        long readoutWidth,
        long readoutHeight,
        double normalizedOffset) noexcept;

    [[nodiscard]] static RECT PlaceAdjacent(
        const TaskbarInfo& taskbar,
        long readoutWidth,
        long readoutHeight,
        double normalizedOffset) noexcept;
};

}  // namespace netstat::windows
