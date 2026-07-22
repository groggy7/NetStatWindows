#include "core/InterfacePolicy.h"
#include "core/RateCalculator.h"
#include "core/RateFormatter.h"
#include "core/Settings.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace {

int failures = 0;

void Expect(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void ExpectNear(
    const double actual,
    const double expected,
    const std::string& message) {
    Expect(std::abs(actual - expected) < 0.000001, message);
}

void ExpectEqual(
    const std::wstring& actual,
    const std::wstring& expected,
    const std::string& message) {
    if (actual != expected) {
        std::wcerr << L"FAIL: " << std::wstring(message.begin(), message.end())
                   << L" expected [" << expected << L"] actual [" << actual
                   << L"]\n";
        ++failures;
    }
}

netstat::NetworkSnapshot Snapshot(
    std::initializer_list<
        std::pair<const netstat::InterfaceId, netstat::InterfaceCounters>> counters,
    const double timestamp) {
    return {std::unordered_map<
                netstat::InterfaceId,
                netstat::InterfaceCounters>(counters),
            timestamp};
}

void TestRateCalculator() {
    netstat::RateCalculator calculator;

    auto rate = calculator.AddSnapshot(
        Snapshot({{1, {100, 200}}, {2, {1000, 2000}}}, 1.0));
    Expect(rate == netstat::NetworkRate{}, "first sample is neutral");

    rate = calculator.AddSnapshot(
        Snapshot({{1, {300, 600}}, {2, {1600, 2800}}}, 3.0));
    ExpectNear(rate.downBytesPerSecond, 400.0, "download uses delta/time");
    ExpectNear(rate.upBytesPerSecond, 600.0, "upload uses delta/time");

    calculator.Reset();
    rate = calculator.AddSnapshot(Snapshot({{1, {500, 800}}}, 4.0));
    Expect(rate == netstat::NetworkRate{}, "reset discards baseline");

    rate = calculator.AddSnapshot(
        Snapshot({{1, {600, 900}}, {3, {50000, 60000}}}, 5.0));
    Expect(rate == netstat::NetworkRate{}, "added interface is neutral");

    rate = calculator.AddSnapshot(
        Snapshot({{1, {700, 1100}}, {3, {50300, 60400}}}, 6.0));
    ExpectNear(rate.downBytesPerSecond, 400.0, "new interface baseline recovers");
    ExpectNear(rate.upBytesPerSecond, 600.0, "new upload baseline recovers");

    rate = calculator.AddSnapshot(
        Snapshot({{1, {1, 2}}, {3, {4, 5}}}, 7.0));
    Expect(rate == netstat::NetworkRate{}, "counter reset is neutral");

    rate = calculator.AddSnapshot(
        Snapshot({{1, {101, 202}}, {3, {304, 405}}}, 8.0));
    ExpectNear(rate.downBytesPerSecond, 400.0, "counter reset establishes baseline");
    ExpectNear(rate.upBytesPerSecond, 600.0, "upload reset establishes baseline");

    rate = calculator.AddSnapshot(
        Snapshot({{1, {201, 302}}, {3, {404, 505}}}, 8.0));
    Expect(rate == netstat::NetworkRate{}, "invalid elapsed time is neutral");

    calculator.Reset();
    rate = calculator.AddSnapshot(
        Snapshot({{1, {0, 0}}, {2, {0, 0}}}, 1.0));
    Expect(rate == netstat::NetworkRate{}, "overflow baseline is neutral");
    rate = calculator.AddSnapshot(Snapshot(
        {{1,
          {std::numeric_limits<std::uint64_t>::max(),
           std::numeric_limits<std::uint64_t>::max()}},
         {2,
          {std::numeric_limits<std::uint64_t>::max(),
           std::numeric_limits<std::uint64_t>::max()}}},
        2.0));
    Expect(rate == netstat::NetworkRate{}, "delta overflow is neutral");

    calculator.Reset();
    rate = calculator.AddSnapshot(Snapshot({{1, {100, 200}}}, 1.0));
    Expect(rate == netstat::NetworkRate{}, "replacement baseline is neutral");
    rate = calculator.AddSnapshot(Snapshot({{2, {200, 400}}}, 2.0));
    Expect(rate == netstat::NetworkRate{}, "replaced interface is neutral");

    rate = calculator.AddSnapshot(
        Snapshot({{2, {300, 600}}}, std::numeric_limits<double>::infinity()));
    Expect(rate == netstat::NetworkRate{}, "invalid timestamp is neutral");
}

void TestRateFormatter() {
    using netstat::RateFormatter;
    using netstat::UnitMode;

    ExpectEqual(RateFormatter::Format(0, UnitMode::Bytes), L"  0 KB/s", "zero");
    ExpectEqual(
        RateFormatter::Format(12400, UnitMode::Bytes),
        L" 12 KB/s",
        "kilobytes rounded");
    ExpectEqual(
        RateFormatter::Format(1500, UnitMode::Bits),
        L" 12 Kb/s",
        "bits conversion");
    ExpectEqual(
        RateFormatter::Format(1260000, UnitMode::Bytes),
        L"1.3 MB/s",
        "megabytes below ten");
    ExpectEqual(
        RateFormatter::Format(2000000, UnitMode::Bytes),
        L"  2 MB/s",
        "whole megabytes");
    ExpectEqual(
        RateFormatter::Format(100000, UnitMode::Bytes),
        L"0.1 MB/s",
        "exact boundary");
    ExpectEqual(
        RateFormatter::Format(99500, UnitMode::Bytes),
        L"0.1 MB/s",
        "rounded boundary");
    ExpectEqual(
        RateFormatter::Format(99499, UnitMode::Bytes),
        L" 99 KB/s",
        "below rounded boundary");
    ExpectEqual(
        RateFormatter::Format(99500000, UnitMode::Bytes),
        L"0.1 GB/s",
        "higher unit boundary");
    ExpectEqual(
        RateFormatter::Format(-1, UnitMode::Bytes),
        L"  0 KB/s",
        "negative is zero");
    ExpectEqual(
        RateFormatter::Format(
            std::numeric_limits<double>::infinity(), UnitMode::Bytes),
        L"  0 KB/s",
        "infinity is zero");
    ExpectEqual(
        RateFormatter::Format(
            std::numeric_limits<double>::quiet_NaN(), UnitMode::Bytes),
        L"  0 KB/s",
        "NaN is zero");
    ExpectEqual(
        RateFormatter::FormatCompact(1260000, UnitMode::Bytes),
        L"1.3M",
        "compact formatter");
}

void TestSettingsValidation() {
    auto settings = netstat::Settings::Defaults();
    Expect(
        settings.displayLayout == netstat::DisplayLayout::Stacked,
        "stacked layout is the default");
    Expect(settings.startAtSignIn, "start at sign-in defaults on");

    settings.updateIntervalSeconds = 3.0;
    settings.customWidthDips = 300.0;
    settings.fontSizeDips = std::numeric_limits<double>::quiet_NaN();
    settings.normalizedOffset = 5.0;
    settings.displayLayout = static_cast<netstat::DisplayLayout>(999);
    settings.Validate();

    const auto defaults = netstat::Settings::Defaults();
    ExpectNear(
        settings.updateIntervalSeconds,
        defaults.updateIntervalSeconds,
        "invalid interval defaults");
    ExpectNear(settings.customWidthDips, 0.0, "invalid width defaults");
    ExpectNear(
        settings.fontSizeDips,
        defaults.fontSizeDips,
        "invalid font defaults");
    ExpectNear(settings.normalizedOffset, 1.0, "offset clamps");
    Expect(
        settings.displayLayout == netstat::DisplayLayout::Stacked,
        "invalid enum defaults");
}

void TestInterfacePolicy() {
    using netstat::InterfaceMetadata;
    using netstat::InterfaceMode;
    using netstat::InterfacePolicy;
    using netstat::NetworkInterfaceType;

    const InterfaceMetadata ethernet{
        .type = NetworkInterfaceType::Ethernet,
        .isOperational = true,
        .isHardware = true,
        .hasConnector = true};
    Expect(
        InterfacePolicy::ShouldInclude(ethernet, InterfaceMode::Physical),
        "physical Ethernet is included");

    const InterfaceMetadata wifi{
        .type = NetworkInterfaceType::Wifi,
        .isOperational = true,
        .isHardware = true,
        .hasConnector = false};
    Expect(
        InterfacePolicy::ShouldInclude(wifi, InterfaceMode::Physical),
        "physical Wi-Fi is included");

    const InterfaceMetadata virtualEthernet{
        .type = NetworkInterfaceType::Ethernet,
        .isOperational = true,
        .isHardware = false,
        .hasConnector = false};
    Expect(
        !InterfacePolicy::ShouldInclude(
            virtualEthernet, InterfaceMode::Physical),
        "virtual Ethernet is excluded from physical mode");
    Expect(
        InterfacePolicy::ShouldInclude(
            virtualEthernet, InterfaceMode::AllActive),
        "virtual Ethernet is included in all-active mode");

    const InterfaceMetadata tunnel{
        .type = NetworkInterfaceType::Tunnel,
        .isOperational = true,
        .isHardware = false,
        .hasConnector = false};
    Expect(
        !InterfacePolicy::ShouldInclude(tunnel, InterfaceMode::Physical),
        "tunnel is excluded from physical mode");
    Expect(
        InterfacePolicy::ShouldInclude(tunnel, InterfaceMode::AllActive),
        "tunnel is included in all-active mode");

    const InterfaceMetadata loopback{
        .type = NetworkInterfaceType::Loopback,
        .isOperational = true,
        .isHardware = false,
        .hasConnector = false};
    Expect(
        !InterfacePolicy::ShouldInclude(loopback, InterfaceMode::AllActive),
        "loopback is always excluded");

    auto disconnected = ethernet;
    disconnected.isOperational = false;
    Expect(
        !InterfacePolicy::ShouldInclude(disconnected, InterfaceMode::Physical),
        "disconnected interface is excluded");
}

}  // namespace

int main() {
    TestRateCalculator();
    TestRateFormatter();
    TestSettingsValidation();
    TestInterfacePolicy();

    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All core tests passed\n";
    return EXIT_SUCCESS;
}
