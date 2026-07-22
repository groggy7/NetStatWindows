#include "platform/WindowsNetworkSnapshotProvider.h"

#include "core/InterfacePolicy.h"

#include <WinSock2.h>
#include <Windows.h>
#include <netioapi.h>
#include <iphlpapi.h>

#include <memory>

namespace netstat::windows {
namespace {

struct MibTableDeleter {
    void operator()(MIB_IF_TABLE2* table) const noexcept {
        if (table != nullptr) {
            ::FreeMibTable(table);
        }
    }
};

using MibTablePointer = std::unique_ptr<MIB_IF_TABLE2, MibTableDeleter>;

[[nodiscard]] NetworkInterfaceType ClassifyType(const ULONG type) noexcept {
    switch (type) {
        case IF_TYPE_ETHERNET_CSMACD:
            return NetworkInterfaceType::Ethernet;
        case IF_TYPE_IEEE80211:
            return NetworkInterfaceType::Wifi;
        case IF_TYPE_SOFTWARE_LOOPBACK:
            return NetworkInterfaceType::Loopback;
        case IF_TYPE_TUNNEL:
            return NetworkInterfaceType::Tunnel;
        default:
            return NetworkInterfaceType::Other;
    }
}

[[nodiscard]] InterfaceMetadata MetadataFor(const MIB_IF_ROW2& row) noexcept {
    return {
        .type = ClassifyType(row.Type),
        .isOperational = row.OperStatus == IfOperStatusUp,
        .isHardware =
            row.InterfaceAndOperStatusFlags.HardwareInterface != FALSE,
        .hasConnector =
            row.InterfaceAndOperStatusFlags.ConnectorPresent != FALSE};
}

}  // namespace

std::optional<NetworkSnapshot> WindowsNetworkSnapshotProvider::Capture(
    const InterfaceMode mode) const noexcept {
    const auto timestamp = clock_.NowSeconds();
    if (!timestamp.has_value()) {
        return std::nullopt;
    }

    MIB_IF_TABLE2* rawTable = nullptr;
    if (::GetIfTable2(&rawTable) != NO_ERROR || rawTable == nullptr) {
        return std::nullopt;
    }
    const MibTablePointer table(rawTable);

    NetworkSnapshot snapshot;
    snapshot.timestampSeconds = *timestamp;
    snapshot.countersByInterface.reserve(table->NumEntries);

    for (ULONG index = 0; index < table->NumEntries; ++index) {
        const auto& row = table->Table[index];
        if (!InterfacePolicy::ShouldInclude(MetadataFor(row), mode)) {
            continue;
        }

        snapshot.countersByInterface.insert_or_assign(
            row.InterfaceLuid.Value,
            InterfaceCounters{
                .receivedBytes = row.InOctets,
                .sentBytes = row.OutOctets});
    }

    return snapshot;
}

}  // namespace netstat::windows
