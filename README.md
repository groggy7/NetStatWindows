# NetStatBar for Windows

NetStatBar for Windows is a small native Windows 11 taskbar utility that shows current download and upload speeds. Its default readout places download above upload to use the taskbar's vertical space efficiently.

The application is implemented in C++20 with Win32, Direct2D, and DirectWrite. It does not capture packets, inspect network contents, require administrator access, make network requests, or collect telemetry.

## Current status

Development is in progress. The portable rate calculation and formatting core can be built on macOS or Windows. The application executable is Windows-only.

## Local core build

```sh
cmake --preset default
cmake --build --preset default
ctest --preset default
```

## Windows build

Open a Developer PowerShell for Visual Studio and run:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-release
ctest --preset windows-debug
```

The project intentionally has no third-party runtime dependencies.
