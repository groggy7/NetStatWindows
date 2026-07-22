#include "app/App.h"

#include "app/Messages.h"
#include "core/RateFormatter.h"

#include <shellapi.h>
#include <windowsx.h>
#include <wtsapi32.h>

#include <string>

namespace netstat::windows {
namespace {

[[nodiscard]] std::wstring InterfaceTitle(const InterfaceMode mode) {
    return mode == InterfaceMode::Physical
        ? L"Physical Wi-Fi/Ethernet"
        : L"All active interfaces";
}

[[nodiscard]] std::wstring IntervalTitle(const double seconds) {
    if (seconds == 0.5) {
        return L"0.5 s";
    }
    return std::to_wstring(static_cast<int>(seconds)) + L" s";
}

}  // namespace

App::App(const HINSTANCE instance)
    : instance_(instance),
      readoutWindow_(instance),
      settingsWindow_(instance),
      samplingWorker_([this](const NetworkRate rate) { PublishRate(rate); }) {}

App::~App() {
    Shutdown();
}

int App::Run(const int showCommand) {
    static_cast<void>(showCommand);
    if (!RegisterWindowClass() || !CreateControllerWindow()) {
        ::MessageBoxW(
            nullptr,
            L"NetStatBar could not create its application window.",
            L"NetStatBar startup failed",
            MB_OK | MB_ICONERROR);
        Shutdown();
        return 1;
    }
    if (!Initialize()) {
        ::MessageBoxW(
            nullptr,
            L"NetStatBar could not add its notification icon.",
            L"NetStatBar startup failed",
            MB_OK | MB_ICONERROR);
        Shutdown();
        return 1;
    }

    MSG message{};
    while (true) {
        const BOOL result = ::GetMessageW(&message, nullptr, 0, 0);
        if (result == 0) {
            return static_cast<int>(message.wParam);
        }
        if (result == -1) {
            return 1;
        }
        if (settingsWindow_.HandleDialogMessage(&message)) {
            continue;
        }
        ::TranslateMessage(&message);
        ::DispatchMessageW(&message);
    }
}

bool App::RegisterWindowClass() const {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance_;
    windowClass.lpszClassName = kControllerWindowClass;
    return ::RegisterClassExW(&windowClass) != 0 ||
        ::GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

bool App::CreateControllerWindow() {
    window_ = ::CreateWindowExW(
        0,
        kControllerWindowClass,
        L"NetStatBar",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        instance_,
        this);
    return window_ != nullptr;
}

bool App::Initialize() {
    settings_ = settingsStore_.Load();
    if (!startupManager_.SetEnabled(settings_.startAtSignIn)) {
        settings_.startAtSignIn = false;
        static_cast<void>(settingsStore_.Save(settings_));
    }
    taskbarCreatedMessage_ = ::RegisterWindowMessageW(L"TaskbarCreated");
    if (!trayIcon_.Add(window_, kTrayCallbackMessage)) {
        return false;
    }
    readoutAvailable_ = readoutWindow_.Create();
    ::WTSRegisterSessionNotification(window_, NOTIFY_FOR_THIS_SESSION);
    UpdateTooltip();
    if (readoutAvailable_) {
        readoutWindow_.Update(displayedRate_, settings_);
    }
    samplingWorker_.Start(
        settings_.interfaceMode, settings_.updateIntervalSeconds);
    return true;
}

void App::Shutdown() noexcept {
    samplingWorker_.Stop();
    readoutWindow_.Destroy();
    readoutAvailable_ = false;
    if (window_ != nullptr) {
        ::WTSUnRegisterSessionNotification(window_);
    }
    trayIcon_.Remove();
}

void App::PublishRate(const NetworkRate rate) {
    {
        const std::scoped_lock lock(rateMutex_);
        pendingRate_ = rate;
    }
    if (window_ != nullptr) {
        ::PostMessageW(window_, kRateUpdatedMessage, 0, 0);
    }
}

void App::ApplyLatestRate() {
    {
        const std::scoped_lock lock(rateMutex_);
        displayedRate_ = pendingRate_;
    }
    UpdateTooltip();
    if (readoutAvailable_) {
        readoutWindow_.Update(displayedRate_, settings_);
    }
}

void App::UpdateTooltip() {
    const std::wstring down = RateFormatter::Format(
        displayedRate_.downBytesPerSecond, settings_.unitMode);
    const std::wstring up = RateFormatter::Format(
        displayedRate_.upBytesPerSecond, settings_.unitMode);
    trayIcon_.UpdateTooltip(
        L"Download: " + down + L"\nUpload: " + up + L"\n" +
        IntervalTitle(settings_.updateIntervalSeconds) + L" \u00b7 " +
        InterfaceTitle(settings_.interfaceMode));
}

void App::HandleTrayCallback(const WPARAM wParam, const LPARAM lParam) {
    const UINT event = LOWORD(lParam);
    if (event == WM_CONTEXTMENU) {
        const POINT anchor{
            GET_X_LPARAM(wParam),
            GET_Y_LPARAM(wParam)};
        const auto command =
            trayIcon_.ShowContextMenu(
                settings_, displayedRate_, paused_, anchor);
        if (command.has_value()) {
            HandleCommand(*command);
        }
    } else if (event == NIN_SELECT || event == NIN_KEYSELECT) {
        ShowSettings();
    }
}

void App::HandleCommand(const TrayCommand command) {
    bool reconfigureSampler = false;
    switch (command) {
        case TrayCommand::Pause:
            paused_ = !paused_;
            if (paused_) {
                samplingWorker_.Stop();
            } else {
                samplingWorker_.Start(
                    settings_.interfaceMode, settings_.updateIntervalSeconds);
            }
            UpdateTooltip();
            return;
        case TrayCommand::IntervalHalfSecond:
            settings_.updateIntervalSeconds = 0.5;
            reconfigureSampler = true;
            break;
        case TrayCommand::IntervalOneSecond:
            settings_.updateIntervalSeconds = 1.0;
            reconfigureSampler = true;
            break;
        case TrayCommand::IntervalTwoSeconds:
            settings_.updateIntervalSeconds = 2.0;
            reconfigureSampler = true;
            break;
        case TrayCommand::IntervalFiveSeconds:
            settings_.updateIntervalSeconds = 5.0;
            reconfigureSampler = true;
            break;
        case TrayCommand::LayoutStacked:
            settings_.displayLayout = DisplayLayout::Stacked;
            break;
        case TrayCommand::LayoutArrows:
            settings_.displayLayout = DisplayLayout::Arrows;
            break;
        case TrayCommand::LayoutLabels:
            settings_.displayLayout = DisplayLayout::Labels;
            break;
        case TrayCommand::LayoutCompact:
            settings_.displayLayout = DisplayLayout::Compact;
            break;
        case TrayCommand::LayoutDownloadOnly:
            settings_.displayLayout = DisplayLayout::DownloadOnly;
            break;
        case TrayCommand::LayoutUploadOnly:
            settings_.displayLayout = DisplayLayout::UploadOnly;
            break;
        case TrayCommand::UnitsBytes:
            settings_.unitMode = UnitMode::Bytes;
            break;
        case TrayCommand::UnitsBits:
            settings_.unitMode = UnitMode::Bits;
            break;
        case TrayCommand::InterfacesPhysical:
            settings_.interfaceMode = InterfaceMode::Physical;
            reconfigureSampler = true;
            break;
        case TrayCommand::InterfacesAllActive:
            settings_.interfaceMode = InterfaceMode::AllActive;
            reconfigureSampler = true;
            break;
        case TrayCommand::PlacementAutomatic:
            settings_.placementMode = PlacementMode::Automatic;
            break;
        case TrayCommand::PlacementOnTaskbar:
            settings_.placementMode = PlacementMode::OnTaskbar;
            break;
        case TrayCommand::PlacementAdjacent:
            settings_.placementMode = PlacementMode::Adjacent;
            break;
        case TrayCommand::PlacementTrayOnly:
            settings_.placementMode = PlacementMode::TrayOnly;
            break;
        case TrayCommand::Reposition:
            BeginReadoutReposition();
            return;
        case TrayCommand::Settings:
            ShowSettings();
            return;
        case TrayCommand::Reset:
            static_cast<void>(settingsStore_.Reset());
            settings_ = Settings::Defaults();
            reconfigureSampler = true;
            break;
        case TrayCommand::Exit:
            ::DestroyWindow(window_);
            return;
    }
    SaveAndRefresh(reconfigureSampler);
    settingsWindow_.Sync(settings_);
}

void App::SaveAndRefresh(const bool reconfigureSampler) {
    settings_.Validate();
    if (!startupManager_.SetEnabled(settings_.startAtSignIn)) {
        settings_.startAtSignIn = false;
    }
    static_cast<void>(settingsStore_.Save(settings_));
    if (reconfigureSampler && !paused_) {
        samplingWorker_.Reconfigure(
            settings_.interfaceMode, settings_.updateIntervalSeconds);
    }
    UpdateTooltip();
    if (readoutAvailable_) {
        readoutWindow_.Update(displayedRate_, settings_);
    }
}

void App::ShowSettings() {
    settingsWindow_.Show(
        nullptr,
        settings_,
        [this](const Settings& settings) {
            ApplySettingsFromWindow(settings);
        },
        [this] { BeginReadoutReposition(); });
}

void App::ApplySettingsFromWindow(const Settings& settings) {
    const bool reconfigureSampler =
        settings.interfaceMode != settings_.interfaceMode ||
        settings.updateIntervalSeconds != settings_.updateIntervalSeconds;
    settings_ = settings;
    SaveAndRefresh(reconfigureSampler);
}

void App::BeginReadoutReposition() {
    if (!readoutAvailable_ ||
        settings_.placementMode == PlacementMode::TrayOnly) {
        ::MessageBoxW(
            nullptr,
            L"Enable a visible readout before repositioning it.",
            L"NetStatBar",
            MB_OK | MB_ICONINFORMATION);
        return;
    }

    ::MessageBoxW(
        nullptr,
        L"Drag the outlined readout along the taskbar and release to save. "
        L"You can also use the arrow keys and press Enter. Press Escape or "
        L"right-click to cancel.",
        L"Reposition NetStatBar",
        MB_OK | MB_ICONINFORMATION);
    const bool started = readoutWindow_.BeginReposition(
        [this](const std::optional<double> offset) {
            if (!offset.has_value()) {
                return;
            }
            if (settingsWindow_.IsOpen()) {
                settingsWindow_.ApplyReposition(*offset);
            } else {
                settings_.placementMode = PlacementMode::OnTaskbar;
                settings_.normalizedOffset = *offset;
                SaveAndRefresh(false);
            }
        });
    if (!started) {
        ::MessageBoxW(
            nullptr,
            L"The readout is not currently available. Show the taskbar and try "
            L"again.",
            L"NetStatBar",
            MB_OK | MB_ICONWARNING);
    }
}

LRESULT CALLBACK App::WindowProcedure(
    const HWND window,
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    App* app = reinterpret_cast<App*>(
        ::GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        app = static_cast<App*>(create->lpCreateParams);
        app->window_ = window;
        ::SetWindowLongPtrW(
            window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
    }

    if (app != nullptr) {
        return app->HandleMessage(message, wParam, lParam);
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

LRESULT App::HandleMessage(
    const UINT message,
    const WPARAM wParam,
    const LPARAM lParam) {
    if (message == taskbarCreatedMessage_ && taskbarCreatedMessage_ != 0) {
        static_cast<void>(trayIcon_.ReAdd());
        if (readoutAvailable_) {
            readoutWindow_.RefreshPlacement(true);
            readoutWindow_.Update(displayedRate_, settings_);
        }
        return 0;
    }

    switch (message) {
        case kTrayCallbackMessage:
            HandleTrayCallback(wParam, lParam);
            return 0;
        case kRateUpdatedMessage:
            ApplyLatestRate();
            return 0;
        case kShowSettingsMessage:
            ShowSettings();
            return 0;
        case WM_POWERBROADCAST:
            if (wParam == PBT_APMRESUMEAUTOMATIC ||
                wParam == PBT_APMRESUMECRITICAL ||
                wParam == PBT_APMRESUMESUSPEND) {
                samplingWorker_.ResetBaseline();
            }
            return TRUE;
        case WM_WTSSESSION_CHANGE:
            if (wParam == WTS_SESSION_LOCK) {
                readoutWindow_.Hide();
            } else if (wParam == WTS_SESSION_UNLOCK) {
                if (!paused_) {
                    samplingWorker_.ResetBaseline();
                }
                if (readoutAvailable_) {
                    readoutWindow_.RefreshPlacement(true);
                    readoutWindow_.Update(displayedRate_, settings_);
                }
            }
            return 0;
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
        case WM_SETTINGCHANGE:
        case WM_THEMECHANGED:
            if (readoutAvailable_) {
                readoutWindow_.RefreshPlacement(true);
                readoutWindow_.Update(displayedRate_, settings_);
            }
            return 0;
        case WM_CLOSE:
            ::DestroyWindow(window_);
            return 0;
        case WM_DESTROY:
            Shutdown();
            window_ = nullptr;
            ::PostQuitMessage(0);
            return 0;
        default:
            return ::DefWindowProcW(window_, message, wParam, lParam);
    }
}

}  // namespace netstat::windows
