#pragma once

#include "app/SamplingWorker.h"
#include "core/NetworkRate.h"
#include "core/Settings.h"
#include "platform/RegistrySettingsStore.h"
#include "platform/StartupManager.h"
#include "ui/ReadoutWindow.h"
#include "ui/SettingsWindow.h"
#include "ui/TrayIcon.h"

#include <Windows.h>

#include <mutex>

namespace netstat::windows {

class App final {
public:
    explicit App(HINSTANCE instance);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    [[nodiscard]] int Run(int showCommand, bool openSettingsOnStart);

private:
    [[nodiscard]] bool RegisterWindowClass() const;
    [[nodiscard]] bool CreateControllerWindow();
    [[nodiscard]] bool Initialize();
    void Shutdown() noexcept;
    void PublishRate(NetworkRate rate);
    void ApplyLatestRate();
    void UpdateTooltip();
    void HandleTrayCallback(WPARAM wParam, LPARAM lParam);
    void HandleCommand(TrayCommand command);
    void SaveAndRefresh(bool reconfigureSampler);
    void ApplySettingsFromWindow(const Settings& settings);
    void BeginReadoutReposition();
    void ShowSettings();

    static LRESULT CALLBACK WindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE instance_{};
    HWND window_{};
    UINT taskbarCreatedMessage_{};
    RegistrySettingsStore settingsStore_;
    StartupManager startupManager_;
    Settings settings_;
    NetworkRate displayedRate_;
    NetworkRate pendingRate_;
    std::mutex rateMutex_;
    TrayIcon trayIcon_;
    ReadoutWindow readoutWindow_;
    SettingsWindow settingsWindow_;
    SamplingWorker samplingWorker_;
    bool paused_{};
    bool readoutAvailable_{};
};

}  // namespace netstat::windows
