#include "app/SingleInstance.h"

#include "app/Messages.h"

namespace netstat::windows {
namespace {

constexpr wchar_t kMutexName[] = L"Local\\NetStatBar.Windows.SingleInstance";

}  // namespace

SingleInstance::SingleInstance() noexcept {
    mutex_ = ::CreateMutexW(nullptr, FALSE, kMutexName);
    isPrimary_ = mutex_ != nullptr && ::GetLastError() != ERROR_ALREADY_EXISTS;
}

SingleInstance::~SingleInstance() {
    if (mutex_ != nullptr) {
        ::CloseHandle(mutex_);
    }
}

bool SingleInstance::IsPrimary() const noexcept {
    return isPrimary_;
}

bool SingleInstance::ActivateExisting() noexcept {
    const HWND existing = ::FindWindowExW(
        HWND_MESSAGE, nullptr, kControllerWindowClass, nullptr);
    return existing != nullptr &&
        ::PostMessageW(existing, kShowSettingsMessage, 0, 0) != FALSE;
}

}  // namespace netstat::windows
