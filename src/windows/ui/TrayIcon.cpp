#include "ui/TrayIcon.h"

#include "core/RateFormatter.h"
#include "resource.h"

#include <shellapi.h>
#include <strsafe.h>

#include <memory>
#include <type_traits>

namespace netstat::windows {
namespace {

constexpr UINT kIconId = 1;

struct MenuDeleter {
    void operator()(HMENU menu) const noexcept {
        if (menu != nullptr) {
            ::DestroyMenu(menu);
        }
    }
};

using MenuPointer =
    std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;

[[nodiscard]] UINT CheckedFlag(const bool checked) noexcept {
    return checked ? MF_CHECKED : MF_UNCHECKED;
}

void AppendCommand(
    const HMENU menu,
    const TrayCommand command,
    const wchar_t* title,
    const bool checked = false) {
    ::AppendMenuW(
        menu,
        MF_STRING | CheckedFlag(checked),
        static_cast<UINT_PTR>(command),
        title);
}

void AppendSubmenu(const HMENU menu, const HMENU submenu, const wchar_t* title) {
    ::AppendMenuW(
        menu,
        MF_POPUP,
        reinterpret_cast<UINT_PTR>(submenu),
        title);
}

[[nodiscard]] std::wstring Summary(
    const NetworkRate& rate,
    const UnitMode mode) {
    return L"\u2193 " + RateFormatter::Format(rate.downBytesPerSecond, mode) +
        L"    \u2191 " + RateFormatter::Format(rate.upBytesPerSecond, mode);
}

}  // namespace

TrayIcon::~TrayIcon() {
    Remove();
}

bool TrayIcon::Add(const HWND owner, const UINT callbackMessage) {
    owner_ = owner;
    callbackMessage_ = callbackMessage;
    icon_ = static_cast<HICON>(::LoadImageW(
        ::GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(IDI_NETSTATBAR),
        IMAGE_ICON,
        0,
        0,
        LR_DEFAULTSIZE | LR_SHARED));
    if (icon_ == nullptr) {
        icon_ = ::LoadIconW(nullptr, IDI_APPLICATION);
    }
    return AddInternal();
}

bool TrayIcon::ReAdd() {
    Remove();
    added_ = false;
    return AddInternal();
}

bool TrayIcon::AddInternal() {
    if (owner_ == nullptr || icon_ == nullptr) {
        return false;
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = owner_;
    data.uID = kIconId;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = callbackMessage_;
    data.hIcon = icon_;
    ::StringCchCopyW(data.szTip, ARRAYSIZE(data.szTip), tooltip_.c_str());

    if (::Shell_NotifyIconW(NIM_ADD, &data) == FALSE) {
        return false;
    }

    data.uVersion = NOTIFYICON_VERSION_4;
    ::Shell_NotifyIconW(NIM_SETVERSION, &data);
    added_ = true;
    return true;
}

void TrayIcon::Remove() noexcept {
    if (!added_ || owner_ == nullptr) {
        return;
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = owner_;
    data.uID = kIconId;
    ::Shell_NotifyIconW(NIM_DELETE, &data);
    added_ = false;
}

void TrayIcon::UpdateTooltip(const std::wstring& tooltip) {
    tooltip_ = tooltip;
    if (!added_) {
        return;
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = owner_;
    data.uID = kIconId;
    data.uFlags = NIF_TIP | NIF_SHOWTIP;
    ::StringCchCopyW(data.szTip, ARRAYSIZE(data.szTip), tooltip_.c_str());
    ::Shell_NotifyIconW(NIM_MODIFY, &data);
}

std::optional<TrayCommand> TrayIcon::ShowContextMenu(
    const Settings& settings,
    const NetworkRate& rate,
    const bool paused,
    POINT anchor) const {
    const MenuPointer menu(::CreatePopupMenu());
    if (!menu) {
        return std::nullopt;
    }

    const std::wstring summary = Summary(rate, settings.unitMode);
    ::AppendMenuW(menu.get(), MF_STRING | MF_DISABLED, 0, summary.c_str());
    ::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    AppendCommand(
        menu.get(), TrayCommand::Pause, paused ? L"Resume" : L"Pause", paused);

    const HMENU intervalMenu = ::CreatePopupMenu();
    AppendCommand(
        intervalMenu,
        TrayCommand::IntervalHalfSecond,
        L"0.5 seconds",
        settings.updateIntervalSeconds == 0.5);
    AppendCommand(
        intervalMenu,
        TrayCommand::IntervalOneSecond,
        L"1 second",
        settings.updateIntervalSeconds == 1.0);
    AppendCommand(
        intervalMenu,
        TrayCommand::IntervalTwoSeconds,
        L"2 seconds",
        settings.updateIntervalSeconds == 2.0);
    AppendCommand(
        intervalMenu,
        TrayCommand::IntervalFiveSeconds,
        L"5 seconds",
        settings.updateIntervalSeconds == 5.0);
    AppendSubmenu(menu.get(), intervalMenu, L"Update interval");

    const HMENU layoutMenu = ::CreatePopupMenu();
    AppendCommand(
        layoutMenu,
        TrayCommand::LayoutStacked,
        L"Stacked",
        settings.displayLayout == DisplayLayout::Stacked);
    AppendCommand(
        layoutMenu,
        TrayCommand::LayoutArrows,
        L"Arrows",
        settings.displayLayout == DisplayLayout::Arrows);
    AppendCommand(
        layoutMenu,
        TrayCommand::LayoutLabels,
        L"Labels",
        settings.displayLayout == DisplayLayout::Labels);
    AppendCommand(
        layoutMenu,
        TrayCommand::LayoutCompact,
        L"Compact",
        settings.displayLayout == DisplayLayout::Compact);
    AppendCommand(
        layoutMenu,
        TrayCommand::LayoutDownloadOnly,
        L"Download only",
        settings.displayLayout == DisplayLayout::DownloadOnly);
    AppendCommand(
        layoutMenu,
        TrayCommand::LayoutUploadOnly,
        L"Upload only",
        settings.displayLayout == DisplayLayout::UploadOnly);
    AppendSubmenu(menu.get(), layoutMenu, L"Layout");

    const HMENU unitsMenu = ::CreatePopupMenu();
    AppendCommand(
        unitsMenu,
        TrayCommand::UnitsBytes,
        L"Bytes/s",
        settings.unitMode == UnitMode::Bytes);
    AppendCommand(
        unitsMenu,
        TrayCommand::UnitsBits,
        L"Bits/s",
        settings.unitMode == UnitMode::Bits);
    AppendSubmenu(menu.get(), unitsMenu, L"Units");

    const HMENU interfacesMenu = ::CreatePopupMenu();
    AppendCommand(
        interfacesMenu,
        TrayCommand::InterfacesPhysical,
        L"Physical Wi-Fi/Ethernet",
        settings.interfaceMode == InterfaceMode::Physical);
    AppendCommand(
        interfacesMenu,
        TrayCommand::InterfacesAllActive,
        L"All active interfaces",
        settings.interfaceMode == InterfaceMode::AllActive);
    AppendSubmenu(menu.get(), interfacesMenu, L"Interfaces");

    const HMENU placementMenu = ::CreatePopupMenu();
    AppendCommand(
        placementMenu,
        TrayCommand::PlacementAutomatic,
        L"Automatic",
        settings.placementMode == PlacementMode::Automatic);
    AppendCommand(
        placementMenu,
        TrayCommand::PlacementOnTaskbar,
        L"On taskbar",
        settings.placementMode == PlacementMode::OnTaskbar);
    AppendCommand(
        placementMenu,
        TrayCommand::PlacementAdjacent,
        L"Adjacent to taskbar",
        settings.placementMode == PlacementMode::Adjacent);
    AppendCommand(
        placementMenu,
        TrayCommand::PlacementTrayOnly,
        L"Tray only",
        settings.placementMode == PlacementMode::TrayOnly);
    AppendSubmenu(menu.get(), placementMenu, L"Placement");

    AppendCommand(menu.get(), TrayCommand::Reposition, L"Reposition readout");
    ::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    AppendCommand(menu.get(), TrayCommand::Settings, L"Settings...");
    AppendCommand(menu.get(), TrayCommand::Reset, L"Reset to defaults");
    ::AppendMenuW(menu.get(), MF_SEPARATOR, 0, nullptr);
    AppendCommand(menu.get(), TrayCommand::Exit, L"Exit");

    if (anchor.x == -1 && anchor.y == -1) {
        ::GetCursorPos(&anchor);
    }
    ::SetForegroundWindow(owner_);
    const UINT selected = ::TrackPopupMenuEx(
        menu.get(),
        TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
        anchor.x,
        anchor.y,
        owner_,
        nullptr);
    ::PostMessageW(owner_, WM_NULL, 0, 0);

    if (selected == 0) {
        return std::nullopt;
    }
    return static_cast<TrayCommand>(selected);
}

}  // namespace netstat::windows
