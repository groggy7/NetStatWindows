#include "platform/TaskbarLocator.h"

#include <shellapi.h>
#include <shellscalingapi.h>

#include <algorithm>

namespace netstat::windows {
namespace {

[[nodiscard]] TaskbarEdge ToEdge(const UINT edge) noexcept {
    switch (edge) {
        case ABE_LEFT:
            return TaskbarEdge::Left;
        case ABE_TOP:
            return TaskbarEdge::Top;
        case ABE_RIGHT:
            return TaskbarEdge::Right;
        case ABE_BOTTOM:
        default:
            return TaskbarEdge::Bottom;
    }
}

[[nodiscard]] long IntersectionThickness(
    const RECT& first,
    const RECT& second,
    const bool horizontal) noexcept {
    if (horizontal) {
        return std::max(
            0L, std::min(first.bottom, second.bottom) -
                    std::max(first.top, second.top));
    }
    return std::max(
        0L, std::min(first.right, second.right) -
                std::max(first.left, second.left));
}

}  // namespace

std::optional<TaskbarInfo> TaskbarLocator::LocatePrimary() const noexcept {
    APPBARDATA data{};
    data.cbSize = sizeof(data);
    if (::SHAppBarMessage(ABM_GETTASKBARPOS, &data) == FALSE) {
        return std::nullopt;
    }

    TaskbarInfo result;
    result.bounds = data.rc;
    result.edge = ToEdge(data.uEdge);
    result.monitor = ::MonitorFromRect(&result.bounds, MONITOR_DEFAULTTOPRIMARY);
    result.shellWindow = ::FindWindowW(L"Shell_TrayWnd", nullptr);
    result.autoHide =
        (::SHAppBarMessage(ABM_GETSTATE, &data) & ABS_AUTOHIDE) != 0;
    result.available = !IsFullscreenOrPresentation();

    if (result.bounds.right <= result.bounds.left ||
        result.bounds.bottom <= result.bounds.top) {
        result.available = false;
    }

    if (result.autoHide && result.shellWindow != nullptr) {
        RECT actual{};
        if (::GetWindowRect(result.shellWindow, &actual) != FALSE) {
            const long expectedThickness = result.IsHorizontal()
                ? result.bounds.bottom - result.bounds.top
                : result.bounds.right - result.bounds.left;
            const long visibleThickness = IntersectionThickness(
                actual, result.bounds, result.IsHorizontal());
            result.available = result.available &&
                visibleThickness >= std::max(4L, expectedThickness / 2);
        }
    }

    return result;
}

bool TaskbarLocator::IsFullscreenOrPresentation() noexcept {
    QUERY_USER_NOTIFICATION_STATE state = QUNS_NOT_PRESENT;
    if (FAILED(::SHQueryUserNotificationState(&state))) {
        return false;
    }
    return state == QUNS_RUNNING_D3D_FULL_SCREEN ||
        state == QUNS_PRESENTATION_MODE;
}

UINT TaskbarLocator::DpiFor(const TaskbarInfo& taskbar) noexcept {
    UINT dpiX = 96;
    UINT dpiY = 96;
    if (taskbar.monitor != nullptr &&
        SUCCEEDED(::GetDpiForMonitor(
            taskbar.monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY))) {
        return dpiX;
    }
    return 96;
}

}  // namespace netstat::windows
