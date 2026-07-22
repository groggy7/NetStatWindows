#include "app/App.h"
#include "app/SingleInstance.h"
#include "platform/RegistrySettingsStore.h"

#include <Windows.h>
#include <CommCtrl.h>
#include <objbase.h>
#include <shellapi.h>

#include <string_view>

namespace {

[[nodiscard]] bool IsStartupLaunch() noexcept {
    int argumentCount = 0;
    wchar_t** arguments = ::CommandLineToArgvW(
        ::GetCommandLineW(), &argumentCount);
    if (arguments == nullptr) {
        return false;
    }

    bool found = false;
    for (int index = 1; index < argumentCount; ++index) {
        if (std::wstring_view(arguments[index]) == L"--startup") {
            found = true;
            break;
        }
    }
    ::LocalFree(arguments);
    return found;
}

}  // namespace

int WINAPI wWinMain(
    const HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    const int showCommand) {
    const bool startupLaunch = IsStartupLaunch();
    if (startupLaunch &&
        !netstat::windows::RegistrySettingsStore().Load().startAtSignIn) {
        return 0;
    }

    const netstat::windows::SingleInstance singleInstance;
    if (!singleInstance.IsValid()) {
        ::MessageBoxW(
            nullptr,
            L"NetStatBar could not create its single-instance lock.",
            L"NetStatBar startup failed",
            MB_OK | MB_ICONERROR);
        return 1;
    }
    if (!singleInstance.IsPrimary()) {
        static_cast<void>(
            netstat::windows::SingleInstance::ActivateExisting());
        return 0;
    }

    const HRESULT comResult =
        ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    const bool uninitializeCom = SUCCEEDED(comResult);

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES;
    ::InitCommonControlsEx(&controls);

    netstat::windows::App app(instance);
    const int exitCode = app.Run(showCommand, !startupLaunch);

    if (uninitializeCom) {
        ::CoUninitialize();
    }
    return exitCode;
}
