#include "platform/StartupManager.h"

#include <Windows.h>
#include <appmodel.h>

#include <array>
#include <vector>

namespace netstat::windows {
namespace {

constexpr wchar_t kRunKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t kRunValue[] = L"NetStatBar";

}  // namespace

bool StartupManager::SetEnabled(const bool enabled) const noexcept {
    if (IsPackaged()) {
        // The package's startup launcher is registered by AppxManifest.xml.
        // It reads the same StartAtSignIn setting before launching the app.
        return true;
    }

    HKEY key = nullptr;
    if (::RegCreateKeyExW(
            HKEY_CURRENT_USER,
            kRunKey,
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            nullptr,
            &key,
            nullptr) != ERROR_SUCCESS) {
        return false;
    }

    LSTATUS status = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring executable = ExecutablePath();
        if (executable.empty()) {
            ::RegCloseKey(key);
            return false;
        }
        const std::wstring command = L"\"" + executable + L"\" --startup";
        status = ::RegSetValueExW(
            key,
            kRunValue,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        status = ::RegDeleteValueW(key, kRunValue);
        if (status == ERROR_FILE_NOT_FOUND) {
            status = ERROR_SUCCESS;
        }
    }
    ::RegCloseKey(key);
    return status == ERROR_SUCCESS;
}

bool StartupManager::IsRegistered() const noexcept {
    if (IsPackaged()) {
        return true;
    }

    DWORD type = 0;
    DWORD size = 0;
    return ::RegGetValueW(
               HKEY_CURRENT_USER,
               kRunKey,
               kRunValue,
               RRF_RT_REG_SZ,
               &type,
               nullptr,
               &size) == ERROR_SUCCESS &&
        size > sizeof(wchar_t);
}

bool StartupManager::IsPackaged() noexcept {
    UINT32 length = 0;
    const LONG status = ::GetCurrentPackageFullName(&length, nullptr);
    return status == ERROR_INSUFFICIENT_BUFFER;
}

std::wstring StartupManager::ExecutablePath() noexcept {
    std::vector<wchar_t> buffer(260);
    while (buffer.size() <= 32768) {
        const DWORD length = ::GetModuleFileNameW(
            nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            return {};
        }
        if (length < buffer.size() - 1) {
            return std::wstring(buffer.data(), length);
        }
        buffer.resize(buffer.size() * 2);
    }
    return {};
}

}  // namespace netstat::windows
