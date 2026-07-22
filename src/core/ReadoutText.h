#pragma once

#include "core/NetworkRate.h"
#include "core/Settings.h"

#include <string>
#include <vector>

namespace netstat {

class ReadoutText final {
public:
    [[nodiscard]] static std::vector<std::wstring> Build(
        const NetworkRate& rate,
        const Settings& settings,
        bool verticalTaskbar,
        bool twoRowsFit);
};

}  // namespace netstat
