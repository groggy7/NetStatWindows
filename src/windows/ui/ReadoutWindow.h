#pragma once

#include "core/NetworkRate.h"
#include "core/Settings.h"
#include "platform/TaskbarGapFinder.h"
#include "platform/TaskbarLocator.h"

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <optional>
#include <functional>

namespace netstat::windows {

class ReadoutWindow final {
public:
    explicit ReadoutWindow(HINSTANCE instance);
    ~ReadoutWindow();

    ReadoutWindow(const ReadoutWindow&) = delete;
    ReadoutWindow& operator=(const ReadoutWindow&) = delete;

    [[nodiscard]] bool Create();
    void Destroy() noexcept;
    void Update(const NetworkRate& rate, const Settings& settings);
    void RefreshPlacement(bool force);
    void Hide() noexcept;
    [[nodiscard]] bool BeginReposition(
        std::function<void(std::optional<double>)> completion);

private:
    [[nodiscard]] bool RegisterWindowClass() const;
    [[nodiscard]] bool CreateFactories();
    [[nodiscard]] bool EnsureSurface(long width, long height, UINT dpi);
    void DestroySurface() noexcept;
    void Render();
    void ShowRenderedSurface();
    void MoveDuringReposition() noexcept;
    void NudgeReposition(long delta) noexcept;
    void FinishReposition(bool save);
    [[nodiscard]] bool PlacementNeedsUpdate(
        const TaskbarInfo& taskbar,
        UINT dpi) const noexcept;
    [[nodiscard]] long AutoWidthPixels(
        const TaskbarInfo& taskbar,
        UINT dpi) const noexcept;

    static LRESULT CALLBACK WindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    HINSTANCE instance_{};
    HWND window_{};
    TaskbarLocator taskbarLocator_;
    TaskbarGapFinder gapFinder_;
    NetworkRate rate_;
    Settings settings_;
    std::optional<TaskbarInfo> lastTaskbar_;
    RECT windowBounds_{};
    UINT dpi_{96};
    bool adjacent_{};
    bool verticalTaskbar_{};
    bool twoRowsFit_{true};
    bool shouldShow_{};
    bool repositioning_{};
    bool dragging_{};
    bool originalAdjacent_{};
    POINT dragOriginCursor_{};
    RECT dragOriginBounds_{};
    RECT originalBounds_{};
    std::function<void(std::optional<double>)> repositionCompletion_;

    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory_;
    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> renderTarget_;
    HDC memoryDc_{};
    HBITMAP bitmap_{};
    HGDIOBJ previousBitmap_{};
    long surfaceWidth_{};
    long surfaceHeight_{};
};

}  // namespace netstat::windows
