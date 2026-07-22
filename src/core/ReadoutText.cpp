#include "core/ReadoutText.h"

#include "core/RateFormatter.h"

namespace netstat {

std::vector<std::wstring> ReadoutText::Build(
    const NetworkRate& rate,
    const Settings& settings,
    const bool verticalTaskbar,
    const bool twoRowsFit) {
    const auto fullDown =
        RateFormatter::Format(rate.downBytesPerSecond, settings.unitMode);
    const auto fullUp =
        RateFormatter::Format(rate.upBytesPerSecond, settings.unitMode);
    const auto compactDown =
        RateFormatter::FormatCompact(rate.downBytesPerSecond, settings.unitMode);
    const auto compactUp =
        RateFormatter::FormatCompact(rate.upBytesPerSecond, settings.unitMode);

    switch (settings.displayLayout) {
        case DisplayLayout::Stacked:
            if (twoRowsFit) {
                if (verticalTaskbar) {
                    return {L"\u2193" + compactDown, L"\u2191" + compactUp};
                }
                return {L"\u2193 " + fullDown, L"\u2191 " + fullUp};
            }
            return {L"\u2193 " + fullDown + L"  \u2191 " + fullUp};
        case DisplayLayout::Arrows:
            return {L"\u2193 " + fullDown + L"  \u2191 " + fullUp};
        case DisplayLayout::Labels:
            return {L"D " + fullDown + L"  U " + fullUp};
        case DisplayLayout::Compact:
            return {fullDown + L" | " + fullUp};
        case DisplayLayout::DownloadOnly:
            return {L"\u2193 " + fullDown};
        case DisplayLayout::UploadOnly:
            return {L"\u2191 " + fullUp};
    }

    return {};
}

}  // namespace netstat
