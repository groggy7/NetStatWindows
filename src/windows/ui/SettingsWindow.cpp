#include "ui/SettingsWindow.h"

#include <CommCtrl.h>
#include <strsafe.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace netstat::windows {
namespace {

constexpr wchar_t kSettingsWindowClass[] = L"NetStatBar.Windows.Settings";

[[nodiscard]] int Scale(const int value, const UINT dpi) noexcept {
    return ::MulDiv(value, static_cast<int>(dpi), 96);
}

void AddComboItem(const HWND combo, const wchar_t* text) {
    ::SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
}

void Move(
    const HWND control,
    const int x,
    const int y,
    const int width,
    const int height,
    const UINT dpi) {
    ::MoveWindow(
        control,
        Scale(x, dpi),
        Scale(y, dpi),
        Scale(width, dpi),
        Scale(height, dpi),
        TRUE);
}

}  // namespace

SettingsWindow::SettingsWindow(const HINSTANCE instance) : instance_(instance) {}

SettingsWindow::~SettingsWindow() {
    CloseWithoutCallback();
}

void SettingsWindow::Show(
    const HWND owner,
    const Settings& settings,
    PreviewCallback previewCallback,
    RepositionCallback repositionCallback) {
    if (window_ != nullptr) {
        ::ShowWindow(window_, SW_RESTORE);
        ::SetForegroundWindow(window_);
        return;
    }
    if (!RegisterWindowClass()) {
        return;
    }

    original_ = settings;
    working_ = settings;
    previewCallback_ = std::move(previewCallback);
    repositionCallback_ = std::move(repositionCallback);
    closing_ = false;

    window_ = ::CreateWindowExW(
        WS_EX_CONTROLPARENT,
        kSettingsWindowClass,
        L"NetStatBar Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        470,
        600,
        owner,
        nullptr,
        instance_,
        this);
    if (window_ == nullptr) {
        return;
    }

    CreateControls();
    const UINT dpi = ::GetDpiForWindow(window_);
    ::SetWindowPos(
        window_,
        nullptr,
        0,
        0,
        Scale(470, dpi),
        Scale(600, dpi),
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    RecreateFont(dpi);
    LayoutControls(dpi);
    PopulateControls();

    RECT windowBounds{};
    ::GetWindowRect(window_, &windowBounds);
    const HMONITOR monitor = ::MonitorFromWindow(
        owner != nullptr ? owner : window_, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (::GetMonitorInfoW(monitor, &monitorInfo) != FALSE) {
        const int width = windowBounds.right - windowBounds.left;
        const int height = windowBounds.bottom - windowBounds.top;
        const int x = monitorInfo.rcWork.left +
            ((monitorInfo.rcWork.right - monitorInfo.rcWork.left - width) / 2);
        const int y = monitorInfo.rcWork.top +
            ((monitorInfo.rcWork.bottom - monitorInfo.rcWork.top - height) / 2);
        ::SetWindowPos(
            window_, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    ::ShowWindow(window_, SW_SHOW);
    ::SetForegroundWindow(window_);
}

void SettingsWindow::Sync(const Settings& settings) {
    if (window_ == nullptr) {
        return;
    }
    original_ = settings;
    working_ = settings;
    PopulateControls();
}

void SettingsWindow::ApplyReposition(const double normalizedOffset) {
    if (window_ == nullptr) {
        return;
    }
    working_.placementMode = PlacementMode::OnTaskbar;
    working_.normalizedOffset = std::clamp(normalizedOffset, 0.0, 1.0);
    PopulateControls();
    if (previewCallback_) {
        previewCallback_(working_);
    }
}

bool SettingsWindow::HandleDialogMessage(MSG* message) const noexcept {
    return window_ != nullptr && message != nullptr &&
        ::IsDialogMessageW(window_, message) != FALSE;
}

bool SettingsWindow::IsOpen() const noexcept {
    return window_ != nullptr;
}

bool SettingsWindow::RegisterWindowClass() const {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance_;
    windowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kSettingsWindowClass;
    return ::RegisterClassExW(&windowClass) != 0 ||
        ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

void SettingsWindow::CreateControls() {
    static_cast<void>(CreateLabel(L"Update interval", LabelUpdateInterval));
    const HWND interval = CreateCombo(UpdateInterval);
    AddComboItem(interval, L"0.5 seconds");
    AddComboItem(interval, L"1 second");
    AddComboItem(interval, L"2 seconds");
    AddComboItem(interval, L"5 seconds");

    static_cast<void>(CreateLabel(L"Display layout", LabelLayout));
    const HWND layout = CreateCombo(Layout);
    AddComboItem(layout, L"Stacked (download above upload)");
    AddComboItem(layout, L"Arrows (single line)");
    AddComboItem(layout, L"Labels (single line)");
    AddComboItem(layout, L"Compact (single line)");
    AddComboItem(layout, L"Download only");
    AddComboItem(layout, L"Upload only");

    static_cast<void>(CreateLabel(L"Units", LabelUnits));
    const HWND units = CreateCombo(Units);
    AddComboItem(units, L"Bytes per second");
    AddComboItem(units, L"Bits per second");

    static_cast<void>(CreateLabel(L"Network interfaces", LabelInterfaces));
    const HWND interfaces = CreateCombo(Interfaces);
    AddComboItem(interfaces, L"Physical Wi-Fi and Ethernet");
    AddComboItem(interfaces, L"All active interfaces");

    static_cast<void>(CreateLabel(L"Readout placement", LabelPlacement));
    const HWND placement = CreateCombo(Placement);
    AddComboItem(placement, L"Automatic safe placement");
    AddComboItem(placement, L"On taskbar (chosen position)");
    AddComboItem(placement, L"Adjacent to taskbar");
    AddComboItem(placement, L"Tray icon only");

    ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Automatic width",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(AutoWidth),
        instance_,
        nullptr);
    ::CreateWindowExW(
        0,
        TRACKBAR_CLASSW,
        L"Readout width",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(WidthSlider),
        instance_,
        nullptr);
    static_cast<void>(CreateLabel(L"", WidthValue));

    static_cast<void>(CreateLabel(L"Font size", LabelFont));
    ::CreateWindowExW(
        0,
        TRACKBAR_CLASSW,
        L"Font size",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_AUTOTICKS,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(FontSlider),
        instance_,
        nullptr);
    static_cast<void>(CreateLabel(L"", FontValue));

    ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Start NetStatBar when I sign in",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(StartAtSignIn),
        instance_,
        nullptr);
    static_cast<void>(CreateLabel(
        L"All active interfaces can count VPN traffic at both the physical "
        L"and tunnel layers.",
        VpnHelp));

    ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Reposition readout...",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(Reposition),
        instance_,
        nullptr);
    ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Reset defaults",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(Reset),
        instance_,
        nullptr);
    ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(Accept),
        instance_,
        nullptr);
    ::CreateWindowExW(
        0,
        WC_BUTTONW,
        L"Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(Cancel),
        instance_,
        nullptr);

    ::SendMessageW(
        Control(WidthSlider),
        TBM_SETRANGE,
        TRUE,
        MAKELPARAM(
            static_cast<int>(Settings::kMinimumWidthDips),
            static_cast<int>(Settings::kMaximumWidthDips)));
    ::SendMessageW(
        Control(FontSlider),
        TBM_SETRANGE,
        TRUE,
        MAKELPARAM(
            static_cast<int>(Settings::kMinimumFontSizeDips),
            static_cast<int>(Settings::kMaximumFontSizeDips)));
}

void SettingsWindow::LayoutControls(const UINT dpi) {
    constexpr int labelX = 20;
    constexpr int controlX = 180;
    constexpr int controlWidth = 250;
    int y = 22;
    Move(Control(LabelUpdateInterval), labelX, y + 4, 145, 24, dpi);
    Move(Control(UpdateInterval), controlX, y, controlWidth, 250, dpi);
    y += 48;
    Move(Control(LabelLayout), labelX, y + 4, 145, 24, dpi);
    Move(Control(Layout), controlX, y, controlWidth, 250, dpi);
    y += 48;
    Move(Control(LabelUnits), labelX, y + 4, 145, 24, dpi);
    Move(Control(Units), controlX, y, controlWidth, 250, dpi);
    y += 48;
    Move(Control(LabelInterfaces), labelX, y + 4, 145, 24, dpi);
    Move(Control(Interfaces), controlX, y, controlWidth, 250, dpi);
    y += 48;
    Move(Control(LabelPlacement), labelX, y + 4, 145, 24, dpi);
    Move(Control(Placement), controlX, y, controlWidth, 250, dpi);
    y += 48;

    Move(Control(AutoWidth), controlX, y, 150, 24, dpi);
    y += 28;
    Move(Control(WidthSlider), controlX, y, 195, 30, dpi);
    Move(Control(WidthValue), 385, y + 4, 45, 22, dpi);
    y += 45;

    Move(Control(LabelFont), labelX, y + 4, 145, 24, dpi);
    Move(Control(FontSlider), controlX, y, 195, 30, dpi);
    Move(Control(FontValue), 385, y + 4, 45, 22, dpi);
    y += 46;

    Move(Control(StartAtSignIn), 20, y, 300, 25, dpi);
    y += 34;
    Move(Control(VpnHelp), 20, y, 410, 42, dpi);
    y += 50;

    Move(Control(Reposition), 20, y, 160, 30, dpi);
    Move(Control(Reset), 190, y, 120, 30, dpi);
    Move(Control(Accept), 325, y, 50, 30, dpi);
    Move(Control(Cancel), 380, y, 70, 30, dpi);
}

void SettingsWindow::RecreateFont(const UINT dpi) {
    if (font_ != nullptr) {
        ::DeleteObject(font_);
    }
    font_ = ::CreateFontW(
        -::MulDiv(9, static_cast<int>(dpi), 72),
        0,
        0,
        0,
        FW_NORMAL,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI");
    for (HWND child = ::GetWindow(window_, GW_CHILD); child != nullptr;
         child = ::GetWindow(child, GW_HWNDNEXT)) {
        ::SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    }
}

void SettingsWindow::PopulateControls() {
    if (window_ == nullptr) {
        return;
    }
    int intervalIndex = 1;
    for (std::size_t index = 0; index < Settings::kUpdateIntervals.size(); ++index) {
        if (working_.updateIntervalSeconds == Settings::kUpdateIntervals[index]) {
            intervalIndex = static_cast<int>(index);
            break;
        }
    }
    ::SendMessageW(Control(UpdateInterval), CB_SETCURSEL, intervalIndex, 0);
    ::SendMessageW(
        Control(Layout),
        CB_SETCURSEL,
        static_cast<WPARAM>(working_.displayLayout),
        0);
    ::SendMessageW(
        Control(Units), CB_SETCURSEL, static_cast<WPARAM>(working_.unitMode), 0);
    ::SendMessageW(
        Control(Interfaces),
        CB_SETCURSEL,
        static_cast<WPARAM>(working_.interfaceMode),
        0);
    ::SendMessageW(
        Control(Placement),
        CB_SETCURSEL,
        static_cast<WPARAM>(working_.placementMode),
        0);

    const bool automaticWidth = working_.customWidthDips == 0.0;
    ::SendMessageW(
        Control(AutoWidth),
        BM_SETCHECK,
        automaticWidth ? BST_CHECKED : BST_UNCHECKED,
        0);
    const int width = automaticWidth
        ? 96
        : static_cast<int>(std::round(working_.customWidthDips));
    ::SendMessageW(Control(WidthSlider), TBM_SETPOS, TRUE, width);
    ::EnableWindow(Control(WidthSlider), !automaticWidth);
    ::SendMessageW(
        Control(FontSlider),
        TBM_SETPOS,
        TRUE,
        static_cast<int>(std::round(working_.fontSizeDips)));
    ::SendMessageW(
        Control(StartAtSignIn),
        BM_SETCHECK,
        working_.startAtSignIn ? BST_CHECKED : BST_UNCHECKED,
        0);
    UpdateValueLabels();
}

void SettingsWindow::CollectControls() {
    const LRESULT intervalIndex =
        ::SendMessageW(Control(UpdateInterval), CB_GETCURSEL, 0, 0);
    if (intervalIndex >= 0 &&
        intervalIndex < static_cast<LRESULT>(Settings::kUpdateIntervals.size())) {
        working_.updateIntervalSeconds =
            Settings::kUpdateIntervals[static_cast<std::size_t>(intervalIndex)];
    }
    working_.displayLayout = static_cast<DisplayLayout>(
        ::SendMessageW(Control(Layout), CB_GETCURSEL, 0, 0));
    working_.unitMode = static_cast<UnitMode>(
        ::SendMessageW(Control(Units), CB_GETCURSEL, 0, 0));
    working_.interfaceMode = static_cast<InterfaceMode>(
        ::SendMessageW(Control(Interfaces), CB_GETCURSEL, 0, 0));
    working_.placementMode = static_cast<PlacementMode>(
        ::SendMessageW(Control(Placement), CB_GETCURSEL, 0, 0));

    const bool automaticWidth =
        ::SendMessageW(Control(AutoWidth), BM_GETCHECK, 0, 0) == BST_CHECKED;
    working_.customWidthDips = automaticWidth
        ? 0.0
        : static_cast<double>(
              ::SendMessageW(Control(WidthSlider), TBM_GETPOS, 0, 0));
    working_.fontSizeDips = static_cast<double>(
        ::SendMessageW(Control(FontSlider), TBM_GETPOS, 0, 0));
    working_.startAtSignIn =
        ::SendMessageW(Control(StartAtSignIn), BM_GETCHECK, 0, 0) ==
        BST_CHECKED;
    working_.Validate();
}

void SettingsWindow::PreviewChanges() {
    CollectControls();
    ::EnableWindow(Control(WidthSlider), working_.customWidthDips != 0.0);
    UpdateValueLabels();
    if (previewCallback_) {
        previewCallback_(working_);
    }
}

void SettingsWindow::AcceptAndClose() {
    PreviewChanges();
    original_ = working_;
    CloseWithoutCallback();
}

void SettingsWindow::CancelAndClose() {
    if (previewCallback_) {
        previewCallback_(original_);
    }
    CloseWithoutCallback();
}

void SettingsWindow::CloseWithoutCallback() noexcept {
    if (closing_) {
        return;
    }
    closing_ = true;
    const HWND window = window_;
    if (window != nullptr) {
        ::DestroyWindow(window);
    }
    window_ = nullptr;
    if (font_ != nullptr) {
        ::DeleteObject(font_);
        font_ = nullptr;
    }
    previewCallback_ = {};
    repositionCallback_ = {};
    closing_ = false;
}

void SettingsWindow::UpdateValueLabels() {
    const bool automaticWidth =
        ::SendMessageW(Control(AutoWidth), BM_GETCHECK, 0, 0) == BST_CHECKED;
    const auto width = ::SendMessageW(Control(WidthSlider), TBM_GETPOS, 0, 0);
    const auto font = ::SendMessageW(Control(FontSlider), TBM_GETPOS, 0, 0);
    wchar_t widthText[32]{};
    if (automaticWidth) {
        static_cast<void>(
            ::StringCchCopyW(widthText, ARRAYSIZE(widthText), L"Auto"));
    } else {
        static_cast<void>(::StringCchPrintfW(
            widthText, ARRAYSIZE(widthText), L"%lld", width));
    }
    ::SetWindowTextW(Control(WidthValue), widthText);

    wchar_t fontText[32]{};
    static_cast<void>(::StringCchPrintfW(
        fontText, ARRAYSIZE(fontText), L"%lld pt", font));
    ::SetWindowTextW(Control(FontValue), fontText);
}

HWND SettingsWindow::CreateLabel(const wchar_t* text, const int id) const {
    return ::CreateWindowExW(
        0,
        WC_STATICW,
        text,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        0,
        0,
        0,
        0,
        window_,
        id == 0
            ? nullptr
            : reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        instance_,
        nullptr);
}

HWND SettingsWindow::CreateCombo(const int id) const {
    return ::CreateWindowExW(
        0,
        WC_COMBOBOXW,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL |
            CBS_DROPDOWNLIST,
        0,
        0,
        0,
        0,
        window_,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        instance_,
        nullptr);
}

HWND SettingsWindow::Control(const int id) const noexcept {
    return ::GetDlgItem(window_, id);
}

LRESULT CALLBACK SettingsWindow::WindowProcedure(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    auto* settings = reinterpret_cast<SettingsWindow*>(
        ::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        settings = static_cast<SettingsWindow*>(create->lpCreateParams);
        settings->window_ = window;
        ::SetWindowLongPtrW(
            window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(settings));
    }
    if (settings != nullptr) {
        return settings->HandleMessage(message, wParam, lParam);
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

LRESULT SettingsWindow::HandleMessage(
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    switch (message) {
        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            const int notification = HIWORD(wParam);
            if (id == Accept && notification == BN_CLICKED) {
                AcceptAndClose();
            } else if (id == Cancel && notification == BN_CLICKED) {
                CancelAndClose();
            } else if (id == Reset && notification == BN_CLICKED) {
                working_ = Settings::Defaults();
                PopulateControls();
                PreviewChanges();
            } else if (id == Reposition && notification == BN_CLICKED) {
                if (repositionCallback_) {
                    repositionCallback_();
                }
            } else if ((id == UpdateInterval || id == Layout || id == Units ||
                        id == Interfaces || id == Placement) &&
                       notification == CBN_SELCHANGE) {
                PreviewChanges();
            } else if ((id == AutoWidth || id == StartAtSignIn) &&
                       notification == BN_CLICKED) {
                PreviewChanges();
            }
            return 0;
        }
        case WM_HSCROLL:
            if (reinterpret_cast<HWND>(lParam) == Control(WidthSlider) ||
                reinterpret_cast<HWND>(lParam) == Control(FontSlider)) {
                PreviewChanges();
            }
            return 0;
        case WM_DPICHANGED: {
            const auto* suggested = reinterpret_cast<RECT*>(lParam);
            ::SetWindowPos(
                window_,
                nullptr,
                suggested->left,
                suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top,
                SWP_NOZORDER | SWP_NOACTIVATE);
            const UINT dpi = HIWORD(wParam);
            RecreateFont(dpi);
            LayoutControls(dpi);
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                CancelAndClose();
                return 0;
            }
            break;
        case WM_CLOSE:
            CancelAndClose();
            return 0;
        case WM_DESTROY:
            return 0;
        default:
            break;
    }
    return ::DefWindowProcW(window_, message, wParam, lParam);
}

}  // namespace netstat::windows
