#pragma once

#include <Windows.h>
#include <d2d1.h>

namespace netstat::windows {

struct ThemeColors {
    D2D1_COLOR_F foreground{};
    D2D1_COLOR_F adjacentBackground{};
    bool highContrast{};
    bool systemUsesLightTheme{};
};

class ThemeMonitor final {
public:
    [[nodiscard]] static ThemeColors Current() noexcept;
};

}  // namespace netstat::windows
