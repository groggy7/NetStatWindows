#pragma once

#include <Windows.h>

namespace netstat::windows {

class SingleInstance final {
public:
    SingleInstance() noexcept;
    ~SingleInstance();

    SingleInstance(const SingleInstance&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;

    [[nodiscard]] bool IsPrimary() const noexcept;
    [[nodiscard]] static bool ActivateExisting() noexcept;

private:
    HANDLE mutex_{};
    bool isPrimary_{};
};

}  // namespace netstat::windows
