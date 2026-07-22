#include "core/InterfacePolicy.h"

namespace netstat {

bool InterfacePolicy::ShouldInclude(
    const InterfaceMetadata& metadata,
    const InterfaceMode mode) noexcept {
    if (!metadata.isOperational ||
        metadata.type == NetworkInterfaceType::Loopback) {
        return false;
    }

    if (mode == InterfaceMode::AllActive) {
        return true;
    }

    const bool supportedPhysicalType =
        metadata.type == NetworkInterfaceType::Ethernet ||
        metadata.type == NetworkInterfaceType::Wifi;
    return supportedPhysicalType &&
        (metadata.isHardware || metadata.hasConnector);
}

}  // namespace netstat
