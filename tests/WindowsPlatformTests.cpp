#include "platform/MonotonicClock.h"
#include "platform/RegistrySettingsStore.h"
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

}  // namespace

int main() {
    TestClockAndSnapshot();
    TestRegistryRoundTrip();
    if (failures != 0) {
        return EXIT_FAILURE;
    }
    std::cout << "All Windows platform tests passed\n";
    return EXIT_SUCCESS;
}
