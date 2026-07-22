# Testing and release gates

## Automated tests

On Windows:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug --parallel
ctest --preset windows-debug
```

The suite covers:

- First-sample baseline behavior.
- Actual elapsed-time rate calculations.
- Read failure, interface churn, counter reset, invalid time, and overflow handling.
- Decimal byte/bit formatting and unit-boundary behavior.
- Physical, virtual, loopback, tunnel, and disconnected interface policy.
- Stacked, compact vertical, single-line, and single-direction text generation.
- Settings defaults and invalid-value recovery.
- Windows performance-counter, live `GetIfTable2`, and isolated registry round trips.

GitHub Actions builds Debug and Release with MSVC, runs CTest, creates an unsigned MSIX, and uploads the binaries/package.

## Required manual matrix

Before a public release, record results for:

- Windows 11 24H2 and every later supported release.
- 100%, 125%, 150%, 200%, and mixed-monitor DPI.
- One, two, and three displays; dock/undock and primary-display changes.
- Taskbar auto-hide, crowded taskbar, light, dark, high contrast, and transparency disabled.
- Explorer graceful restart and forced restart.
- Sleep, hibernate, session lock, and resume.
- Wi-Fi, Ethernet, USB Ethernet, adapter handoff, VPN, Hyper-V, and WSL.
- Fullscreen video and a Direct3D fullscreen application.
- Mouse-only and keyboard-only settings/reposition workflows.
- Clean MSIX install, update, startup opt-in/out, and uninstall.

## Soak and performance gates

Use a Release build at the one-second interval. The release candidate must:

- Run for seven days without growing USER/GDI handles or private memory continuously.
- Average below 0.2% CPU on a representative idle x64 laptop.
- Remain below 30 MB private working set after warm-up.
- Avoid disk writes on each sample.
- Recover after Explorer restart and repeated sleep/resume cycles.
- Never show a false spike on adapter or VPN transitions.

## Release blockers

Do not release if:

- The readout remains topmost while the taskbar is hidden for fullscreen use.
- The notification icon does not recover after Explorer restarts.
- Automatic placement covers an accessible taskbar control instead of falling back.
- A formatted value is clipped without a complete tray tooltip alternative.
- Start at sign-in cannot be disabled in-app.
- The package is unsigned or its publisher does not match the signing certificate.
