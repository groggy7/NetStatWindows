#include "platform/ThemeMonitor.h"

#include <d2d1helper.h>

namespace netstat::windows {
namespace {

[[nodiscard]] D2D1_COLOR_F ColorFromSystem(const int index) noexcept {
    const COLORREF color = ::GetSysColor(index);
    return D2D1::ColorF(
        static_cast<float>(GetRValue(color)) / 255.0F,
        static_cast<float>(GetGValue(color)) / 255.0F,
        static_cast<float>(GetBValue(color)) / 255.0F,
        1.0F);
}

[[nodiscard]] bool SystemUsesLightTheme() noexcept {
    DWORD value = 1;
    DWORD size = sizeof(value);
    const LSTATUS status = ::RegGetValueW(
        HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        L"SystemUsesLightTheme",
        RRF_RT_REG_DWORD,
        nullptr,
        &value,
        &size);
    return status != ERROR_SUCCESS || value != 0;
}

}  // namespace

ThemeColors ThemeMonitor::Current() noexcept {
    HIGHCONTRASTW highContrast{};
    highContrast.cbSize = sizeof(highContrast);
    const bool isHighContrast =
        ::SystemParametersInfoW(
            SPI_GETHIGHCONTRAST,
            sizeof(highContrast),
            &highContrast,
            0) != FALSE &&
        (highContrast.dwFlags & HCF_HIGHCONTRASTON) != 0;
    const bool isLight = SystemUsesLightTheme();

    if (isHighContrast) {
        return {
            .foreground = ColorFromSystem(COLOR_WINDOWTEXT),
            .adjacentBackground = ColorFromSystem(COLOR_WINDOW),
            .highContrast = true,
            .systemUsesLightTheme = isLight};
    }

    return {
        .foreground = isLight
            ? D2D1::ColorF(0x111111, 1.0F)
            : D2D1::ColorF(0xFFFFFF, 1.0F),
        .adjacentBackground = isLight
            ? D2D1::ColorF(0xF3F3F3, 0.92F)
            : D2D1::ColorF(0x202020, 0.92F),
        .highContrast = false,
        .systemUsesLightTheme = isLight};
}

}  // namespace netstat::windows
