#pragma once

#include <Windows.h>

namespace netstat::windows {

inline constexpr UINT kTrayCallbackMessage = WM_APP + 1;
inline constexpr UINT kRateUpdatedMessage = WM_APP + 2;
inline constexpr UINT kShowSettingsMessage = WM_APP + 3;
inline constexpr wchar_t kControllerWindowClass[] =
    L"NetStatBar.Windows.Controller";

}  // namespace netstat::windows
