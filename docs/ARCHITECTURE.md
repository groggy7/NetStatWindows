# Architecture

## Process model

`NetStatBar.exe` is a single-instance, per-user Win32 GUI process. It creates:

- A message-only controller window for Shell, power, session, and worker notifications.
- A standard `Shell_NotifyIcon` notification icon and native popup menu.
- A layered, no-activate readout window rendered through Direct2D and DirectWrite.
- A modeless settings window made from Win32 common controls.
- One sampling worker thread.

`NetStatBarStartup.exe` is a small MSIX startup launcher. It reads the user's `StartAtSignIn` setting and starts the main executable only when the user has opted in. It immediately exits after launching and is not a watchdog.

## Layers

### Portable core

`src/core` has no dependency on Windows headers:

- `RateCalculator` converts validated counter snapshots to rates.
- `RateFormatter` implements the same decimal formatting behavior as the macOS app.
- `InterfacePolicy` selects physical or all-active interfaces from normalized metadata.
- `ReadoutText` produces directionally explicit one- or two-row strings.
- `Settings` owns defaults and validation.

This layer is compiled and tested on both macOS and Windows.

### Windows platform

`src/windows/platform` isolates operating-system APIs:

- `WindowsNetworkSnapshotProvider` calls `GetIfTable2`, reads 64-bit octet counters, and keys rows by interface LUID.
- `MonotonicClock` wraps `QueryPerformanceCounter`.
- `RegistrySettingsStore` persists validated typed values under `HKCU\Software\NetStatBar`.
- `StartupManager` manages the current user's unpackaged startup registration and recognizes packaged execution.
- `TaskbarLocator` uses `SHAppBarMessage` and monitor/DPI state.
- `TaskbarGapFinder` uses UI Automation bounding rectangles to avoid interactive taskbar controls.
- `ThemeMonitor` supplies system light/dark and high-contrast colors.

### Windows UI and application

`src/windows/ui` contains the notification icon, readout, and settings window. `src/windows/app` owns lifecycle, sampling, single-instance activation, and startup entry points.

## Sampling lifecycle

The worker establishes a baseline, waits for the configured interval, captures another snapshot, and divides byte deltas by actual monotonic elapsed time. A failed read, changed interface set, decreased counter, invalid time, or overflow produces a neutral sample and replaces the baseline.

The baseline is also reset on:

- Interface policy or interval changes.
- Sleep/resume.
- Session unlock.
- Explicit worker restart.

Only immutable rates are handed to the UI thread. No history is written to disk.

## Taskbar placement

Automatic mode obtains the taskbar rectangle, enumerates accessible interactive controls only when geometry changes, merges their occupied intervals, and chooses the closest safe gap to the user's preferred normalized offset. If no gap fits, it places the readout immediately adjacent to the taskbar's inner edge with a theme-aware background.

On-taskbar mode uses the saved offset directly. The readout remains a separate top-level window; it is never an Explorer child. Normal hit testing returns `HTTRANSPARENT`.

## Failure behavior

- If the notification icon cannot be added, startup fails visibly instead of leaving an uncontrollable background process.
- If Direct2D or the readout surface fails, tray-only operation remains available.
- Explorer's `TaskbarCreated` message recreates the notification icon and refreshes placement.
- Fullscreen presentation and hidden auto-hide states suppress the readout.
- Corrupt individual settings fall back independently to defaults.
