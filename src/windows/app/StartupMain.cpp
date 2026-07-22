#include "platform/RegistrySettingsStore.h"

#include <Windows.h>

#include <string>
#include <vector>

namespace {

[[nodiscard]] std::wstring ModuleDirectory() {
    std::vector<wchar_t> buffer(260);
    while (buffer.size() <= 32768) {
        const DWORD length = ::GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            std::wstring path(buffer.data(), length);
            const auto separator = path.find_last_of(L"\\/");
            return separator == std::wstring::npos
                ? std::wstring{}
                : path.substr(0, separator);
        }
        buffer.resize(buffer.size() * 2);
    }
    return {};
}

}  // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    const netstat::windows::RegistrySettingsStore store;
    if (!store.Load().startAtSignIn) {
        return 0;
    }

    const std::wstring directory = ModuleDirectory();
    if (directory.empty()) {
        return 1;
    }
    const std::wstring executable = directory + L"\\NetStatBar.exe";
    std::wstring command = L"\"" + executable + L"\" --startup";

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    const BOOL created = ::CreateProcessW(
        executable.c_str(),
        command.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        directory.c_str(),
        &startup,
        &process);
    if (created == FALSE) {
        return 1;
    }
    ::CloseHandle(process.hThread);
    ::CloseHandle(process.hProcess);
    return 0;
}
