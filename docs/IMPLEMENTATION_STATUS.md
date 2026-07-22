# Implementation status

The native x64 MVP implementation is complete in this repository.

## Completed milestones

1. Independent Git repository and CMake build.
2. Portable rate calculation, formatter, settings, interface policy, and tests.
3. Windows `GetIfTable2` sampling, monotonic timing, and registry persistence.
4. Single-instance host, notification icon/menu, worker lifecycle, sleep/session handling, and Explorer recovery.
5. Direct2D/DirectWrite stacked readout, taskbar geometry, safe-gap placement, adjacent fallback, DPI, theme, high contrast, auto-hide, and fullscreen behavior.
6. Native modeless settings, live preview and rollback, startup opt-in, mouse/keyboard repositioning, and keyboard navigation.
7. Version/icon resources, portable and MSIX packaging, packaged startup launcher, Windows CI, architecture, build, test, and privacy documentation.

## Verification state

- Portable core compiles with warnings as errors and passes locally on macOS.
- The complete x64 Windows application, startup launcher, resources, and Windows tests cross-compile and link with MinGW-w64.
- MSVC Debug/Release builds, Windows runtime tests, MSIX schema validation, signing, and the manual compatibility matrix run in Windows CI or on Windows release machines.

## Post-MVP backlog

- Native ARM64 release artifacts and hardware validation.
- Persistent readouts on every secondary-monitor taskbar.
- Localization after the English resource vocabulary stabilizes.
- Historical charts or per-process accounting only if added as separately reviewed products; neither belongs in the privacy-preserving MVP.
