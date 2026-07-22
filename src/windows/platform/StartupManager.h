#pragma once

#include <string>

namespace netstat::windows {

class StartupManager final {
public:
    [[nodiscard]] bool SetEnabled(bool enabled) const noexcept;
    [[nodiscard]] bool IsRegistered() const noexcept;
    [[nodiscard]] static bool IsPackaged() noexcept;

private:
    [[nodiscard]] static std::wstring ExecutablePath() noexcept;
};

}  // namespace netstat::windows
