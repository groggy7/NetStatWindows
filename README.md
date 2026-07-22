# NetStatBar for Windows

NetStatBar is a small native Windows 11 taskbar utility that displays current download and upload rates. The default layout puts download above upload so both values fit naturally within a horizontal taskbar.

```text
↓ 12 MB/s
↑ 850 KB/s
```

It is implemented in C++20 with Win32, Direct2D, and DirectWrite. There is no Electron, WebView, .NET runtime, packet-capture driver, service, telemetry, or network request.

## Features

- Stacked, arrow, label, compact, download-only, and upload-only layouts.
- Update intervals of 0.5, 1, 2, or 5 seconds.
- Decimal bytes/s or bits/s units.
- Physical Wi-Fi/Ethernet or all-active-interface totals.
- Automatic, on-taskbar, adjacent, and tray-only placement modes.
- Automatic safe-gap detection with an adjacent fallback when the taskbar is crowded.
- Custom width and font size with a live native settings window.
- Mouse or keyboard readout repositioning.
- Per-monitor DPI rendering, dark mode, high contrast, auto-hide, fullscreen suppression, and Explorer restart recovery.
- Optional start at sign-in.
- No administrator access and no third-party runtime dependencies in the MSVC release build.

## Platform

The initial release target is 64-bit Windows 11 24H2 or later. The primary-monitor taskbar is fully supported. Nonstandard shell replacements, taskbar-modification utilities, and secondary-monitor taskbars are best effort.

Windows does not expose a supported arbitrary-width text slot in the notification area. NetStatBar therefore uses a standard notification icon for interaction and a separate transparent companion window for the readout. It never injects code into Explorer or reparents a window into the taskbar.

## Use

Right-click the NetStatBar notification icon for quick settings, or left-click it to open the full settings window. The persistent readout is click-through during normal use so it does not steal taskbar clicks.

To choose a taskbar position, select **Reposition readout**. Drag and release, or use the arrow keys and press Enter. Escape or right-click cancels.

When **All active interfaces** is selected, VPN traffic can be counted at both the physical and tunnel layers. This is inherent to interface-counter aggregation; use the physical mode when you want physical-adapter throughput.

## Build and test

See [Building](docs/BUILDING.md) and [Testing](docs/TESTING.md). A normal Windows development loop is:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug --parallel
ctest --preset windows-debug
```

The platform-neutral core can also be built and tested on macOS:

```sh
cmake --preset default
cmake --build --preset default --parallel
ctest --preset default
```

## Privacy

NetStatBar reads cumulative byte counters maintained by Windows. It does not inspect traffic contents or retain a usage history. See the complete [privacy statement](docs/PRIVACY.md).

## Project documents

- [Architecture](docs/ARCHITECTURE.md)
- [Building and packaging](docs/BUILDING.md)
- [Testing and release gates](docs/TESTING.md)
- [Privacy](docs/PRIVACY.md)
- [Implementation status](docs/IMPLEMENTATION_STATUS.md)
