#include "platform/MonotonicClock.h"
#include "platform/RegistrySettingsStore.h"
#include "platform/TaskbarGapFinder.h"
#include "platform/WindowsNetworkSnapshotProvider.h"

#include <Windows.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void Expect(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void TestClockAndSnapshot() {
    const netstat::windows::MonotonicClock clock;
    const auto first = clock.NowSeconds();
    const auto second = clock.NowSeconds();
    Expect(first.has_value(), "performance counter is available");
    Expect(second.has_value(), "performance counter remains available");
    if (first.has_value() && second.has_value()) {
        Expect(*second >= *first, "performance counter is monotonic");
    }

    const netstat::windows::WindowsNetworkSnapshotProvider provider;
    const auto snapshot = provider.Capture(netstat::InterfaceMode::AllActive);
    Expect(snapshot.has_value(), "GetIfTable2 returns a snapshot");
}

void TestRegistryRoundTrip() {
    const std::wstring testKey =
        L"Software\\NetStatBarTests\\" + std::to_wstring(::GetCurrentProcessId());
    const netstat::windows::RegistrySettingsStore store(
        HKEY_CURRENT_USER, testKey);
    Expect(store.Reset(), "test registry key starts clean");

    auto settings = netstat::Settings::Defaults();
    settings.updateIntervalSeconds = 2.0;
    settings.displayLayout = netstat::DisplayLayout::Compact;
    settings.unitMode = netstat::UnitMode::Bits;
    settings.interfaceMode = netstat::InterfaceMode::AllActive;
    settings.placementMode = netstat::PlacementMode::Adjacent;
    settings.customWidthDips = 160.0;
    settings.fontSizeDips = 14.0;
    settings.normalizedOffset = 0.25;
    settings.startAtSignIn = false;
    Expect(store.Save(settings), "settings save succeeds");

    const auto loaded = store.Load();
    Expect(loaded.updateIntervalSeconds == 2.0, "interval round trips");
    Expect(
        loaded.displayLayout == netstat::DisplayLayout::Compact,
        "layout round trips");
    Expect(loaded.unitMode == netstat::UnitMode::Bits, "units round trip");
    Expect(
        loaded.interfaceMode == netstat::InterfaceMode::AllActive,
        "interface mode round trips");
    Expect(
        loaded.placementMode == netstat::PlacementMode::Adjacent,
        "placement round trips");
    Expect(loaded.customWidthDips == 160.0, "width round trips");
    Expect(loaded.fontSizeDips == 14.0, "font size round trips");
    Expect(loaded.normalizedOffset == 0.25, "offset round trips");
    Expect(!loaded.startAtSignIn, "startup preference round trips");
    Expect(store.Reset(), "test registry key is removed");
}

void TestTaskbarPlacementMath() {
    netstat::windows::TaskbarInfo bottom;
    bottom.bounds = {0, 1040, 1920, 1080};
    bottom.edge = netstat::windows::TaskbarEdge::Bottom;
    const RECT centered = netstat::windows::TaskbarGapFinder::PlaceAtOffset(
        bottom, 100, 36, 0.5);
    Expect(centered.left == 910, "horizontal placement centers by offset");
    Expect(centered.top == 1042, "horizontal placement centers in taskbar");

    const RECT adjacent = netstat::windows::TaskbarGapFinder::PlaceAdjacent(
        bottom, 100, 36, 0.5);
    Expect(adjacent.top == 1004, "bottom adjacent placement is above taskbar");
    Expect(adjacent.bottom == 1040, "bottom adjacent placement meets taskbar");

    netstat::windows::TaskbarInfo right;
    right.bounds = {1880, 0, 1920, 1080};
    right.edge = netstat::windows::TaskbarEdge::Right;
    const RECT vertical = netstat::windows::TaskbarGapFinder::PlaceAtOffset(
        right, 36, 80, 1.0);
    Expect(vertical.left == 1882, "vertical placement centers in taskbar");
    Expect(vertical.top == 1000, "vertical placement honors end offset");

    const RECT verticalAdjacent =
        netstat::windows::TaskbarGapFinder::PlaceAdjacent(
            right, 36, 80, 1.0);
    Expect(
        verticalAdjacent.right == 1880,
        "right adjacent placement ends at taskbar");
}

}  // namespace

int main() {
    TestClockAndSnapshot();
    TestRegistryRoundTrip();
    TestTaskbarPlacementMath();
    if (failures != 0) {
        return EXIT_FAILURE;
    }
    std::cout << "All Windows platform tests passed\n";
    return EXIT_SUCCESS;
}
