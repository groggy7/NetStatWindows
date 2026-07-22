#include "app/App.h"
#include "app/SingleInstance.h"

#include <Windows.h>
#include <CommCtrl.h>
#include <objbase.h>

int WINAPI wWinMain(
    const HINSTANCE instance,
    HINSTANCE,
    PWSTR,
    const int showCommand) {
    const netstat::windows::SingleInstance singleInstance;
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
    const int exitCode = app.Run(showCommand);

    if (uninitializeCom) {
        ::CoUninitialize();
    }
    return exitCode;
}
