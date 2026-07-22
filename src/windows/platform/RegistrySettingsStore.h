#pragma once

#include "core/Settings.h"

#include <Windows.h>

#include <string>

namespace netstat::windows {

class RegistrySettingsStore final {
public:
    RegistrySettingsStore();
    RegistrySettingsStore(HKEY rootKey, std::wstring subkey);

    [[nodiscard]] Settings Load() const noexcept;
    [[nodiscard]] bool Save(const Settings& settings) const noexcept;
    [[nodiscard]] bool Reset() const noexcept;

private:
    HKEY rootKey_;
    std::wstring subkey_;
};

}  // namespace netstat::windows
