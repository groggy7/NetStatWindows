#pragma once

#include "core/Settings.h"

namespace netstat {

enum class NetworkInterfaceType {
    Other,
    Ethernet,
    Wifi,
    Loopback,
    Tunnel,
};

struct InterfaceMetadata {
    NetworkInterfaceType type{NetworkInterfaceType::Other};
    bool isOperational{};
    bool isHardware{};
    bool hasConnector{};
};

class InterfacePolicy final {
public:
    [[nodiscard]] static bool ShouldInclude(
        const InterfaceMetadata& metadata,
        InterfaceMode mode) noexcept;
};

}  // namespace netstat
