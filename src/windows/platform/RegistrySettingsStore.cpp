#include "platform/RegistrySettingsStore.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>

namespace netstat::windows {
namespace {

constexpr wchar_t kDefaultSubkey[] = L"Software\\NetStatBar";

[[nodiscard]] bool ReadDword(
    const HKEY key,
    const wchar_t* name,
    DWORD& value) noexcept {
    DWORD size = sizeof(value);
    return ::RegGetValueW(
               key,
               nullptr,
               name,
               RRF_RT_REG_DWORD,
               nullptr,
               &value,
               &size) == ERROR_SUCCESS;
}

[[nodiscard]] bool WriteDword(
    const HKEY key,
    const wchar_t* name,
    const DWORD value) noexcept {
    return ::RegSetValueExW(
               key,
               name,
               0,
               REG_DWORD,
               reinterpret_cast<const BYTE*>(&value),
               sizeof(value)) == ERROR_SUCCESS;
}

[[nodiscard]] DWORD ScaledDword(const double value, const double scale) noexcept {
    const double scaled = std::round(value * scale);
    if (!std::isfinite(scaled) || scaled <= 0.0) {
        return 0;
    }
    if (scaled >= static_cast<double>(std::numeric_limits<DWORD>::max())) {
        return std::numeric_limits<DWORD>::max();
    }
    return static_cast<DWORD>(scaled);
}

}  // namespace

RegistrySettingsStore::RegistrySettingsStore()
    : RegistrySettingsStore(HKEY_CURRENT_USER, kDefaultSubkey) {}

RegistrySettingsStore::RegistrySettingsStore(
    const HKEY rootKey,
    std::wstring subkey)
    : rootKey_(rootKey), subkey_(std::move(subkey)) {}

Settings RegistrySettingsStore::Load() const noexcept {
    Settings settings = Settings::Defaults();
    HKEY rawKey = nullptr;
    if (::RegOpenKeyExW(
            rootKey_, subkey_.c_str(), 0, KEY_QUERY_VALUE, &rawKey) !=
        ERROR_SUCCESS) {
        return settings;
    }

    const auto closeKey = [](HKEY key) noexcept { ::RegCloseKey(key); };
    const std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(closeKey)> key(
        rawKey, closeKey);
    DWORD value = 0;

    if (ReadDword(key.get(), L"SchemaVersion", value)) {
        settings.schemaVersion = value;
    }
    if (ReadDword(key.get(), L"UpdateIntervalMs", value)) {
        settings.updateIntervalSeconds = static_cast<double>(value) / 1000.0;
    }
    if (ReadDword(key.get(), L"DisplayLayout", value)) {
        settings.displayLayout = static_cast<DisplayLayout>(value);
    }
    if (ReadDword(key.get(), L"UnitMode", value)) {
        settings.unitMode = static_cast<UnitMode>(value);
    }
    if (ReadDword(key.get(), L"InterfaceMode", value)) {
        settings.interfaceMode = static_cast<InterfaceMode>(value);
    }
    if (ReadDword(key.get(), L"PlacementMode", value)) {
        settings.placementMode = static_cast<PlacementMode>(value);
    }
    if (ReadDword(key.get(), L"MonitorMode", value)) {
        settings.monitorMode = static_cast<MonitorMode>(value);
    }
    if (ReadDword(key.get(), L"CustomWidthCentidips", value)) {
        settings.customWidthDips = static_cast<double>(value) / 100.0;
    }
    if (ReadDword(key.get(), L"FontSizeCentidips", value)) {
        settings.fontSizeDips = static_cast<double>(value) / 100.0;
    }
    if (ReadDword(key.get(), L"OffsetMillionths", value)) {
        settings.normalizedOffset = static_cast<double>(value) / 1'000'000.0;
    }
    if (ReadDword(key.get(), L"StartAtSignIn", value)) {
        settings.startAtSignIn = value != 0;
    }

    settings.Validate();
    return settings;
}

bool RegistrySettingsStore::Save(const Settings& source) const noexcept {
    Settings settings = source;
    settings.Validate();

    HKEY rawKey = nullptr;
    if (::RegCreateKeyExW(
            rootKey_,
            subkey_.c_str(),
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            nullptr,
            &rawKey,
            nullptr) != ERROR_SUCCESS) {
        return false;
    }

    const auto closeKey = [](HKEY key) noexcept { ::RegCloseKey(key); };
    const std::unique_ptr<std::remove_pointer_t<HKEY>, decltype(closeKey)> key(
        rawKey, closeKey);

    bool succeeded = true;
    const auto write = [&](const wchar_t* name, const DWORD value) {
        if (!WriteDword(key.get(), name, value)) {
            succeeded = false;
        }
    };

    write(L"SchemaVersion", settings.schemaVersion);
    write(
        L"UpdateIntervalMs",
        ScaledDword(settings.updateIntervalSeconds, 1000.0));
    write(L"DisplayLayout", static_cast<DWORD>(settings.displayLayout));
    write(L"UnitMode", static_cast<DWORD>(settings.unitMode));
    write(L"InterfaceMode", static_cast<DWORD>(settings.interfaceMode));
    write(L"PlacementMode", static_cast<DWORD>(settings.placementMode));
    write(L"MonitorMode", static_cast<DWORD>(settings.monitorMode));
    write(
        L"CustomWidthCentidips",
        ScaledDword(settings.customWidthDips, 100.0));
    write(
        L"FontSizeCentidips",
        ScaledDword(settings.fontSizeDips, 100.0));
    write(
        L"OffsetMillionths",
        ScaledDword(settings.normalizedOffset, 1'000'000.0));
    write(L"StartAtSignIn", settings.startAtSignIn ? 1U : 0U);
    return succeeded;
}

bool RegistrySettingsStore::Reset() const noexcept {
    const LSTATUS status = ::RegDeleteTreeW(rootKey_, subkey_.c_str());
    return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}

}  // namespace netstat::windows
