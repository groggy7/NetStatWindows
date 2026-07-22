#include "ui/ReadoutWindow.h"

#include "core/ReadoutText.h"
#include "platform/ThemeMonitor.h"

#include <d2d1helper.h>

#include <algorithm>
#include <cmath>
#include <string_view>
#include <utility>

namespace netstat::windows {
namespace {

constexpr wchar_t kReadoutWindowClass[] = L"NetStatBar.Windows.Readout";

[[nodiscard]] bool EqualRect(
    const RECT& first,
    const RECT& second) noexcept {
    return first.left == second.left && first.top == second.top &&
        first.right == second.right && first.bottom == second.bottom;
}

[[nodiscard]] long DipsToPixels(const double dips, const UINT dpi) noexcept {
    return std::max(
        1L, static_cast<long>(std::round(dips * static_cast<double>(dpi) / 96.0)));
}

[[nodiscard]] float PixelsToDips(const long pixels, const UINT dpi) noexcept {
    return static_cast<float>(pixels) * 96.0F / static_cast<float>(dpi);
}

[[nodiscard]] long Width(const RECT& rectangle) noexcept {
    return rectangle.right - rectangle.left;
}

[[nodiscard]] long Height(const RECT& rectangle) noexcept {
    return rectangle.bottom - rectangle.top;
}

}  // namespace

ReadoutWindow::ReadoutWindow(const HINSTANCE instance) : instance_(instance) {}

ReadoutWindow::~ReadoutWindow() {
    Destroy();
}

bool ReadoutWindow::Create() {
    if (!RegisterWindowClass() || !CreateFactories()) {
        return false;
    }

    window_ = ::CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
            WS_EX_TRANSPARENT,
        kReadoutWindowClass,
        L"NetStatBar network speed",
        WS_POPUP,
        0,
        0,
        1,
        1,
        nullptr,
        nullptr,
        instance_,
        this);
    return window_ != nullptr;
}

void ReadoutWindow::Destroy() noexcept {
    Hide();
    DestroySurface();
    if (window_ != nullptr) {
        ::DestroyWindow(window_);
        window_ = nullptr;
    }
    writeFactory_.Reset();
    d2dFactory_.Reset();
}

void ReadoutWindow::Update(
    const NetworkRate& rate,
    const Settings& settings) {
    rate_ = rate;
    const bool settingsChanged =
        settings.displayLayout != settings_.displayLayout ||
        settings.unitMode != settings_.unitMode ||
        settings.placementMode != settings_.placementMode ||
        settings.monitorMode != settings_.monitorMode ||
        settings.customWidthDips != settings_.customWidthDips ||
        settings.fontSizeDips != settings_.fontSizeDips ||
        settings.normalizedOffset != settings_.normalizedOffset;
    settings_ = settings;
    RefreshPlacement(settingsChanged);
    Render();
}

void ReadoutWindow::RefreshPlacement(const bool force) {
    if (window_ == nullptr ||
        settings_.placementMode == PlacementMode::TrayOnly) {
        shouldShow_ = false;
        Hide();
        return;
    }

    const auto taskbar = taskbarLocator_.LocatePrimary();
    if (!taskbar.has_value() || !taskbar->available) {
        shouldShow_ = false;
        Hide();
        return;
    }

    const UINT dpi = TaskbarLocator::DpiFor(*taskbar);
    shouldShow_ = true;
    if (!force && !PlacementNeedsUpdate(*taskbar, dpi)) {
        return;
    }

    dpi_ = dpi;
    verticalTaskbar_ = !taskbar->IsHorizontal();
    const long taskbarThickness = taskbar->IsHorizontal()
        ? Height(taskbar->bounds)
        : Width(taskbar->bounds);
    const long inset = DipsToPixels(2.0, dpi_);
    long readoutWidth = AutoWidthPixels(*taskbar, dpi_);
    long readoutHeight = DipsToPixels(
        std::max(28.0, settings_.fontSizeDips * 2.35), dpi_);

    if (taskbar->IsHorizontal()) {
        readoutHeight = std::max(1L, taskbarThickness - (2 * inset));
    } else {
        readoutWidth = std::max(1L, taskbarThickness - (2 * inset));
    }

    const double availableHeightDips = PixelsToDips(readoutHeight, dpi_);
    twoRowsFit_ = availableHeightDips >= settings_.fontSizeDips * 2.15;
    adjacent_ = settings_.placementMode == PlacementMode::Adjacent;

    std::optional<RECT> safePlacement;
    if (settings_.placementMode == PlacementMode::Automatic) {
        safePlacement = gapFinder_.FindSafeGap(
            *taskbar,
            readoutWidth,
            readoutHeight,
            settings_.normalizedOffset);
        adjacent_ = !safePlacement.has_value();
    }

    if (safePlacement.has_value()) {
        windowBounds_ = *safePlacement;
    } else if (adjacent_) {
        windowBounds_ = TaskbarGapFinder::PlaceAdjacent(
            *taskbar,
            readoutWidth,
            readoutHeight,
            settings_.normalizedOffset);
    } else {
        windowBounds_ = TaskbarGapFinder::PlaceAtOffset(
            *taskbar,
            readoutWidth,
            readoutHeight,
            settings_.normalizedOffset);
    }

    lastTaskbar_ = taskbar;
    if (!EnsureSurface(Width(windowBounds_), Height(windowBounds_), dpi_)) {
        shouldShow_ = false;
        Hide();
    }
}

void ReadoutWindow::Hide() noexcept {
    if (window_ != nullptr) {
        ::ShowWindow(window_, SW_HIDE);
    }
}

bool ReadoutWindow::BeginReposition(
    std::function<void(std::optional<double>)> completion) {
    if (window_ == nullptr || !shouldShow_ || !lastTaskbar_.has_value() ||
        repositioning_) {
        return false;
    }

    repositioning_ = true;
    originalBounds_ = windowBounds_;
    originalAdjacent_ = adjacent_;
    repositionCompletion_ = std::move(completion);
    adjacent_ = false;
    windowBounds_ = TaskbarGapFinder::PlaceAtOffset(
        *lastTaskbar_,
        surfaceWidth_,
        surfaceHeight_,
        settings_.normalizedOffset);

    LONG_PTR extendedStyle = ::GetWindowLongPtrW(window_, GWL_EXSTYLE);
    extendedStyle &= ~(WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
    ::SetWindowLongPtrW(window_, GWL_EXSTYLE, extendedStyle);
    ::SetWindowPos(
        window_,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    ::SetForegroundWindow(window_);
    ::SetFocus(window_);
    Render();
    return true;
}

bool ReadoutWindow::RegisterWindowClass() const {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance_;
    windowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = kReadoutWindowClass;
    return ::RegisterClassExW(&windowClass) != 0 ||
        ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool ReadoutWindow::CreateFactories() {
    if (FAILED(::D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            d2dFactory_.GetAddressOf()))) {
        return false;
    }

    return SUCCEEDED(::DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf())));
}

bool ReadoutWindow::EnsureSurface(
    const long width,
    const long height,
    const UINT dpi) {
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (memoryDc_ != nullptr && surfaceWidth_ == width &&
        surfaceHeight_ == height) {
        renderTarget_->SetDpi(static_cast<float>(dpi), static_cast<float>(dpi));
        return true;
    }

    DestroySurface();
    memoryDc_ = ::CreateCompatibleDC(nullptr);
    if (memoryDc_ == nullptr) {
        return false;
    }

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    bitmap_ = ::CreateDIBSection(
        memoryDc_, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap_ == nullptr || pixels == nullptr) {
        DestroySurface();
        return false;
    }
    previousBitmap_ = ::SelectObject(memoryDc_, bitmap_);

    const D2D1_RENDER_TARGET_PROPERTIES properties =
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                D2D1_ALPHA_MODE_PREMULTIPLIED),
            static_cast<float>(dpi),
            static_cast<float>(dpi));
    if (FAILED(d2dFactory_->CreateDCRenderTarget(
            &properties, &renderTarget_))) {
        DestroySurface();
        return false;
    }

    surfaceWidth_ = width;
    surfaceHeight_ = height;
    return true;
}

void ReadoutWindow::DestroySurface() noexcept {
    renderTarget_.Reset();
    if (memoryDc_ != nullptr && previousBitmap_ != nullptr) {
        ::SelectObject(memoryDc_, previousBitmap_);
    }
    previousBitmap_ = nullptr;
    if (bitmap_ != nullptr) {
        ::DeleteObject(bitmap_);
        bitmap_ = nullptr;
    }
    if (memoryDc_ != nullptr) {
        ::DeleteDC(memoryDc_);
        memoryDc_ = nullptr;
    }
    surfaceWidth_ = 0;
    surfaceHeight_ = 0;
}

void ReadoutWindow::Render() {
    if (!shouldShow_ || window_ == nullptr || renderTarget_ == nullptr ||
        memoryDc_ == nullptr) {
        return;
    }

    RECT drawBounds{0, 0, surfaceWidth_, surfaceHeight_};
    if (FAILED(renderTarget_->BindDC(memoryDc_, &drawBounds))) {
        Hide();
        return;
    }

    const ThemeColors colors = ThemeMonitor::Current();
    renderTarget_->BeginDraw();
    renderTarget_->Clear(
        adjacent_ ? colors.adjacentBackground : D2D1::ColorF(0, 0.0F));

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    HRESULT result = renderTarget_->CreateSolidColorBrush(
        colors.foreground, &brush);

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    if (SUCCEEDED(result)) {
        result = writeFactory_->CreateTextFormat(
            L"Segoe UI Variable Text",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            static_cast<float>(settings_.fontSizeDips),
            L"en-US",
            &format);
    }
    if (FAILED(result)) {
        result = writeFactory_->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            static_cast<float>(settings_.fontSizeDips),
            L"en-US",
            &format);
    }

    const auto lines = ReadoutText::Build(
        rate_, settings_, verticalTaskbar_, twoRowsFit_);
    if (SUCCEEDED(result) && format != nullptr && brush != nullptr &&
        !lines.empty()) {
        format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

        const float widthDips = PixelsToDips(surfaceWidth_, dpi_);
        const float heightDips = PixelsToDips(surfaceHeight_, dpi_);
        const float rowHeight = heightDips / static_cast<float>(lines.size());
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const D2D1_RECT_F rowBounds = D2D1::RectF(
                0.0F,
                rowHeight * static_cast<float>(index),
                widthDips,
                rowHeight * static_cast<float>(index + 1));
            renderTarget_->DrawText(
                lines[index].c_str(),
                static_cast<UINT32>(lines[index].size()),
                format.Get(),
                rowBounds,
                brush.Get(),
                D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }
        if (repositioning_) {
            renderTarget_->DrawRectangle(
                D2D1::RectF(
                    0.5F,
                    0.5F,
                    PixelsToDips(surfaceWidth_, dpi_) - 0.5F,
                    PixelsToDips(surfaceHeight_, dpi_) - 0.5F),
                brush.Get(),
                1.0F);
        }
    }

    result = renderTarget_->EndDraw();
    if (FAILED(result)) {
        DestroySurface();
        lastTaskbar_.reset();
        Hide();
        return;
    }
    ShowRenderedSurface();
}

void ReadoutWindow::MoveDuringReposition() noexcept {
    if (!repositioning_ || !dragging_ || !lastTaskbar_.has_value()) {
        return;
    }
    POINT cursor{};
    if (::GetCursorPos(&cursor) == FALSE) {
        return;
    }

    const long deltaX = cursor.x - dragOriginCursor_.x;
    const long deltaY = cursor.y - dragOriginCursor_.y;
    const auto& taskbar = *lastTaskbar_;
    if (taskbar.IsHorizontal()) {
        const long width = Width(dragOriginBounds_);
        const long left = std::clamp(
            dragOriginBounds_.left + deltaX,
            taskbar.bounds.left,
            taskbar.bounds.right - width);
        windowBounds_.left = left;
        windowBounds_.right = left + width;
    } else {
        const long height = Height(dragOriginBounds_);
        const long top = std::clamp(
            dragOriginBounds_.top + deltaY,
            taskbar.bounds.top,
            taskbar.bounds.bottom - height);
        windowBounds_.top = top;
        windowBounds_.bottom = top + height;
    }
    ShowRenderedSurface();
}

void ReadoutWindow::NudgeReposition(const long delta) noexcept {
    if (!repositioning_ || !lastTaskbar_.has_value()) {
        return;
    }
    const auto& taskbar = *lastTaskbar_;
    if (taskbar.IsHorizontal()) {
        const long width = Width(windowBounds_);
        const long left = std::clamp(
            windowBounds_.left + delta,
            taskbar.bounds.left,
            taskbar.bounds.right - width);
        windowBounds_.left = left;
        windowBounds_.right = left + width;
    } else {
        const long height = Height(windowBounds_);
        const long top = std::clamp(
            windowBounds_.top + delta,
            taskbar.bounds.top,
            taskbar.bounds.bottom - height);
        windowBounds_.top = top;
        windowBounds_.bottom = top + height;
    }
    ShowRenderedSurface();
}

void ReadoutWindow::FinishReposition(const bool save) {
    if (!repositioning_) {
        return;
    }
    if (dragging_) {
        ::ReleaseCapture();
        dragging_ = false;
    }

    std::optional<double> normalizedOffset;
    if (save && lastTaskbar_.has_value()) {
        const auto& taskbar = *lastTaskbar_;
        if (taskbar.IsHorizontal()) {
            const long available =
                Width(taskbar.bounds) - Width(windowBounds_);
            normalizedOffset = available > 0
                ? static_cast<double>(windowBounds_.left - taskbar.bounds.left) /
                    static_cast<double>(available)
                : 0.0;
        } else {
            const long available =
                Height(taskbar.bounds) - Height(windowBounds_);
            normalizedOffset = available > 0
                ? static_cast<double>(windowBounds_.top - taskbar.bounds.top) /
                    static_cast<double>(available)
                : 0.0;
        }
    } else {
        windowBounds_ = originalBounds_;
        adjacent_ = originalAdjacent_;
    }

    repositioning_ = false;
    LONG_PTR extendedStyle = ::GetWindowLongPtrW(window_, GWL_EXSTYLE);
    extendedStyle |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;
    ::SetWindowLongPtrW(window_, GWL_EXSTYLE, extendedStyle);
    ::SetWindowPos(
        window_,
        HWND_TOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    Render();

    auto completion = std::move(repositionCompletion_);
    repositionCompletion_ = {};
    if (completion) {
        completion(normalizedOffset);
    }
}

void ReadoutWindow::ShowRenderedSurface() {
    POINT destination{windowBounds_.left, windowBounds_.top};
    SIZE size{surfaceWidth_, surfaceHeight_};
    POINT source{0, 0};
    BLENDFUNCTION blend{};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    const HDC screen = ::GetDC(nullptr);
    if (screen == nullptr) {
        return;
    }
    const BOOL updated = ::UpdateLayeredWindow(
        window_,
        screen,
        &destination,
        &size,
        memoryDc_,
        &source,
        0,
        &blend,
        ULW_ALPHA);
    ::ReleaseDC(nullptr, screen);
    if (updated != FALSE) {
        ::SetWindowPos(
            window_,
            HWND_TOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
}

bool ReadoutWindow::PlacementNeedsUpdate(
    const TaskbarInfo& taskbar,
    const UINT dpi) const noexcept {
    return !lastTaskbar_.has_value() ||
        renderTarget_ == nullptr ||
        !EqualRect(lastTaskbar_->bounds, taskbar.bounds) ||
        lastTaskbar_->edge != taskbar.edge || dpi_ != dpi;
}

long ReadoutWindow::AutoWidthPixels(
    const TaskbarInfo& taskbar,
    const UINT dpi) const noexcept {
    if (settings_.customWidthDips > 0.0) {
        return DipsToPixels(settings_.customWidthDips, dpi);
    }
    if (!taskbar.IsHorizontal()) {
        return DipsToPixels(60.0, dpi);
    }
    switch (settings_.displayLayout) {
        case DisplayLayout::Stacked:
        case DisplayLayout::DownloadOnly:
        case DisplayLayout::UploadOnly:
            return DipsToPixels(96.0, dpi);
        case DisplayLayout::Arrows:
        case DisplayLayout::Labels:
        case DisplayLayout::Compact:
            return DipsToPixels(180.0, dpi);
    }
    return DipsToPixels(96.0, dpi);
}

LRESULT CALLBACK ReadoutWindow::WindowProcedure(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    auto* readout = reinterpret_cast<ReadoutWindow*>(
        ::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        readout = static_cast<ReadoutWindow*>(create->lpCreateParams);
        ::SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(readout));
    }

    switch (message) {
        case WM_NCHITTEST:
            return readout != nullptr && readout->repositioning_
                ? HTCLIENT
                : HTTRANSPARENT;
        case WM_SETCURSOR:
            if (readout != nullptr && readout->repositioning_) {
                ::SetCursor(::LoadCursorW(nullptr, IDC_SIZEALL));
                return TRUE;
            }
            break;
        case WM_LBUTTONDOWN:
            if (readout != nullptr && readout->repositioning_) {
                readout->dragging_ = true;
                ::GetCursorPos(&readout->dragOriginCursor_);
                readout->dragOriginBounds_ = readout->windowBounds_;
                ::SetCapture(window);
                return 0;
            }
            break;
        case WM_MOUSEMOVE:
            if (readout != nullptr) {
                readout->MoveDuringReposition();
            }
            return 0;
        case WM_LBUTTONUP:
            if (readout != nullptr && readout->repositioning_) {
                readout->FinishReposition(true);
                return 0;
            }
            break;
        case WM_RBUTTONDOWN:
            if (readout != nullptr && readout->repositioning_) {
                readout->FinishReposition(false);
                return 0;
            }
            break;
        case WM_KEYDOWN:
            if (readout != nullptr && readout->repositioning_) {
                if (wParam == VK_ESCAPE) {
                    readout->FinishReposition(false);
                    return 0;
                }
                if (wParam == VK_RETURN || wParam == VK_SPACE) {
                    readout->FinishReposition(true);
                    return 0;
                }
                const long step = (::GetKeyState(VK_SHIFT) & 0x8000) != 0
                    ? 10L
                    : 1L;
                if (wParam == VK_LEFT || wParam == VK_UP) {
                    readout->NudgeReposition(-step);
                    return 0;
                }
                if (wParam == VK_RIGHT || wParam == VK_DOWN) {
                    readout->NudgeReposition(step);
                    return 0;
                }
            }
            break;
        case WM_ERASEBKGND:
            return 1;
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
            if (readout != nullptr) {
                readout->RefreshPlacement(true);
                readout->Render();
            }
            return 0;
        default:
            return ::DefWindowProcW(window, message, wParam, lParam);
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

}  // namespace netstat::windows
