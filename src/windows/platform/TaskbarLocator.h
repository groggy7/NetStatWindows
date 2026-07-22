#pragma once

#include <Windows.h>

#include <optional>

namespace netstat::windows {

enum class TaskbarEdge {
    Left,
    Top,
    Right,
    Bottom,
};

struct TaskbarInfo {
    RECT bounds{};
    TaskbarEdge edge{TaskbarEdge::Bottom};
    HMONITOR monitor{};
    HWND shellWindow{};
    bool autoHide{};
    bool available{};

    [[nodiscard]] bool IsHorizontal() const noexcept {
        return edge == TaskbarEdge::Top || edge == TaskbarEdge::Bottom;
    }
};

class TaskbarLocator final {
public:
    [[nodiscard]] std::optional<TaskbarInfo> LocatePrimary() const noexcept;
    [[nodiscard]] static bool IsFullscreenOrPresentation() noexcept;
    [[nodiscard]] static UINT DpiFor(const TaskbarInfo& taskbar) noexcept;
};

}  // namespace netstat::windows
