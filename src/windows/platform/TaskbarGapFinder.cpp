#include "platform/TaskbarGapFinder.h"

#include <UIAutomation.h>
#include <wrl/client.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace netstat::windows {
namespace {

using Microsoft::WRL::ComPtr;

struct Interval {
    long start{};
    long end{};
};

[[nodiscard]] bool IsInteractiveControl(const CONTROLTYPEID type) noexcept {
    switch (type) {
        case UIA_ButtonControlTypeId:
        case UIA_CheckBoxControlTypeId:
        case UIA_ComboBoxControlTypeId:
        case UIA_EditControlTypeId:
        case UIA_GroupControlTypeId:
        case UIA_HyperlinkControlTypeId:
        case UIA_ListItemControlTypeId:
        case UIA_MenuItemControlTypeId:
        case UIA_RadioButtonControlTypeId:
        case UIA_SplitButtonControlTypeId:
        case UIA_TabItemControlTypeId:
        case UIA_TextControlTypeId:
        case UIA_ToolBarControlTypeId:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] bool Intersects(const RECT& item, const RECT& bounds) noexcept {
    return item.right > bounds.left && item.left < bounds.right &&
        item.bottom > bounds.top && item.top < bounds.bottom;
}

[[nodiscard]] RECT RectFromAxis(
    const TaskbarInfo& taskbar,
    const long axisStart,
    const long width,
    const long height) noexcept {
    if (taskbar.IsHorizontal()) {
        const long top = taskbar.bounds.top +
            ((taskbar.bounds.bottom - taskbar.bounds.top - height) / 2);
        return {axisStart, top, axisStart + width, top + height};
    }

    const long left = taskbar.bounds.left +
        ((taskbar.bounds.right - taskbar.bounds.left - width) / 2);
    return {left, axisStart, left + width, axisStart + height};
}

}  // namespace

std::optional<RECT> TaskbarGapFinder::FindSafeGap(
    const TaskbarInfo& taskbar,
    const long readoutWidth,
    const long readoutHeight,
    const double preferredNormalizedOffset) const noexcept {
    ComPtr<IUIAutomation> automation;
    if (FAILED(::CoCreateInstance(
            CLSID_CUIAutomation,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&automation)))) {
        return std::nullopt;
    }

    ComPtr<IUIAutomationElement> root;
    HRESULT rootResult = E_FAIL;
    if (taskbar.shellWindow != nullptr) {
        rootResult = automation->ElementFromHandle(
            taskbar.shellWindow, &root);
    }
    if (FAILED(rootResult)) {
        rootResult = automation->GetRootElement(&root);
    }
    if (FAILED(rootResult)) {
        return std::nullopt;
    }

    ComPtr<IUIAutomationCondition> condition;
    if (FAILED(automation->CreateTrueCondition(&condition))) {
        return std::nullopt;
    }

    ComPtr<IUIAutomationElementArray> elements;
    if (FAILED(root->FindAll(TreeScope_Descendants, condition.Get(), &elements))) {
        return std::nullopt;
    }

    int count = 0;
    if (FAILED(elements->get_Length(&count))) {
        return std::nullopt;
    }

    std::vector<Interval> occupied;
    occupied.reserve(64);
    constexpr long margin = 4;
    const int cappedCount = std::min(count, 5000);
    for (int index = 0; index < cappedCount; ++index) {
        ComPtr<IUIAutomationElement> element;
        if (FAILED(elements->GetElement(index, &element))) {
            continue;
        }

        CONTROLTYPEID controlType = 0;
        RECT bounds{};
        if (FAILED(element->get_CurrentControlType(&controlType)) ||
            !IsInteractiveControl(controlType) ||
            FAILED(element->get_CurrentBoundingRectangle(&bounds)) ||
            bounds.right <= bounds.left || bounds.bottom <= bounds.top ||
            !Intersects(bounds, taskbar.bounds)) {
            continue;
        }

        const long taskbarAxisLength = taskbar.IsHorizontal()
            ? taskbar.bounds.right - taskbar.bounds.left
            : taskbar.bounds.bottom - taskbar.bounds.top;
        const long itemAxisLength = taskbar.IsHorizontal()
            ? bounds.right - bounds.left
            : bounds.bottom - bounds.top;
        const bool isContainer =
            controlType == UIA_GroupControlTypeId ||
            controlType == UIA_ToolBarControlTypeId;
        if (isContainer && itemAxisLength * 10 >= taskbarAxisLength * 9) {
            continue;
        }

        if (taskbar.IsHorizontal()) {
            occupied.push_back({
                bounds.left - margin,
                bounds.right + margin});
        } else {
            occupied.push_back({
                bounds.top - margin,
                bounds.bottom + margin});
        }
    }

    if (occupied.empty()) {
        return std::nullopt;
    }

    const long axisStart = taskbar.IsHorizontal()
        ? taskbar.bounds.left
        : taskbar.bounds.top;
    const long axisEnd = taskbar.IsHorizontal()
        ? taskbar.bounds.right
        : taskbar.bounds.bottom;
    const long required =
        taskbar.IsHorizontal() ? readoutWidth : readoutHeight;
    if (axisEnd - axisStart < required) {
        return std::nullopt;
    }

    std::sort(occupied.begin(), occupied.end(), [](const auto& left, const auto& right) {
        return left.start < right.start;
    });
    std::vector<Interval> merged;
    for (auto interval : occupied) {
        interval.start = std::clamp(interval.start, axisStart, axisEnd);
        interval.end = std::clamp(interval.end, axisStart, axisEnd);
        if (interval.end <= interval.start) {
            continue;
        }
        if (!merged.empty() && interval.start <= merged.back().end) {
            merged.back().end = std::max(merged.back().end, interval.end);
        } else {
            merged.push_back(interval);
        }
    }

    const double preferred = static_cast<double>(axisStart) +
        std::clamp(preferredNormalizedOffset, 0.0, 1.0) *
            static_cast<double>(axisEnd - axisStart);
    bool found = false;
    long bestStart = axisStart;
    double bestDistance = std::numeric_limits<double>::max();
    long cursor = axisStart;

    const auto considerGap = [&](const long gapStart, const long gapEnd) {
        if (gapEnd - gapStart < required) {
            return;
        }
        const long candidate = std::clamp(
            static_cast<long>(std::round(preferred - required / 2.0)),
            gapStart,
            gapEnd - required);
        const double center = candidate + required / 2.0;
        const double distance = std::abs(center - preferred);
        if (!found || distance < bestDistance) {
            found = true;
            bestStart = candidate;
            bestDistance = distance;
        }
    };

    for (const auto& interval : merged) {
        considerGap(cursor, interval.start);
        cursor = std::max(cursor, interval.end);
    }
    considerGap(cursor, axisEnd);

    if (!found) {
        return std::nullopt;
    }
    return RectFromAxis(
        taskbar, bestStart, readoutWidth, readoutHeight);
}

RECT TaskbarGapFinder::PlaceAtOffset(
    const TaskbarInfo& taskbar,
    const long readoutWidth,
    const long readoutHeight,
    const double normalizedOffset) noexcept {
    const long axisStart = taskbar.IsHorizontal()
        ? taskbar.bounds.left
        : taskbar.bounds.top;
    const long axisEnd = taskbar.IsHorizontal()
        ? taskbar.bounds.right
        : taskbar.bounds.bottom;
    const long required =
        taskbar.IsHorizontal() ? readoutWidth : readoutHeight;
    const long available = std::max(0L, axisEnd - axisStart - required);
    const long position = axisStart + static_cast<long>(std::round(
        std::clamp(normalizedOffset, 0.0, 1.0) * available));
    return RectFromAxis(taskbar, position, readoutWidth, readoutHeight);
}

RECT TaskbarGapFinder::PlaceAdjacent(
    const TaskbarInfo& taskbar,
    const long readoutWidth,
    const long readoutHeight,
    const double normalizedOffset) noexcept {
    RECT result = PlaceAtOffset(
        taskbar, readoutWidth, readoutHeight, normalizedOffset);
    switch (taskbar.edge) {
        case TaskbarEdge::Bottom:
            result.top = taskbar.bounds.top - readoutHeight;
            result.bottom = taskbar.bounds.top;
            break;
        case TaskbarEdge::Top:
            result.top = taskbar.bounds.bottom;
            result.bottom = taskbar.bounds.bottom + readoutHeight;
            break;
        case TaskbarEdge::Left:
            result.left = taskbar.bounds.right;
            result.right = taskbar.bounds.right + readoutWidth;
            break;
        case TaskbarEdge::Right:
            result.left = taskbar.bounds.left - readoutWidth;
            result.right = taskbar.bounds.left;
            break;
    }
    return result;
}

}  // namespace netstat::windows
