#pragma once

#include "core/Settings.h"

#include <Windows.h>

#include <functional>

namespace netstat::windows {

class SettingsWindow final {
public:
    using PreviewCallback = std::function<void(const Settings&)>;
    using RepositionCallback = std::function<void()>;

    explicit SettingsWindow(HINSTANCE instance);
    ~SettingsWindow();

    SettingsWindow(const SettingsWindow&) = delete;
    SettingsWindow& operator=(const SettingsWindow&) = delete;

    void Show(
        HWND owner,
        const Settings& settings,
        PreviewCallback previewCallback,
        RepositionCallback repositionCallback);
    void Sync(const Settings& settings);
    void ApplyReposition(double normalizedOffset);
    [[nodiscard]] bool HandleDialogMessage(MSG* message) const noexcept;
    [[nodiscard]] bool IsOpen() const noexcept;

private:
    enum ControlId : int {
        UpdateInterval = 1001,
        Layout = 1002,
        Units = 1003,
        Interfaces = 1004,
        Placement = 1005,
        AutoWidth = 1006,
        WidthSlider = 1007,
        WidthValue = 1008,
        FontSlider = 1009,
        FontValue = 1010,
        StartAtSignIn = 1011,
        Reposition = 1012,
        Reset = 1013,
        Accept = IDOK,
        Cancel = IDCANCEL,
        LabelUpdateInterval = 1101,
        LabelLayout = 1102,
        LabelUnits = 1103,
        LabelInterfaces = 1104,
        LabelPlacement = 1105,
        LabelFont = 1106,
        VpnHelp = 1107,
    };

    [[nodiscard]] bool RegisterWindowClass() const;
    void CreateControls();
    void LayoutControls(UINT dpi);
    void RecreateFont(UINT dpi);
    void PopulateControls();
    void CollectControls();
    void PreviewChanges();
    void AcceptAndClose();
    void CancelAndClose();
    void CloseWithoutCallback() noexcept;
    void UpdateValueLabels();
    [[nodiscard]] HWND CreateLabel(const wchar_t* text, int id = 0) const;
    [[nodiscard]] HWND CreateCombo(int id) const;
    [[nodiscard]] HWND Control(int id) const noexcept;

    static LRESULT CALLBACK WindowProcedure(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE instance_{};
    HWND window_{};
    HFONT font_{};
    Settings original_;
    Settings working_;
    PreviewCallback previewCallback_;
    RepositionCallback repositionCallback_;
    bool closing_{};
};

}  // namespace netstat::windows
