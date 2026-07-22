# Building and packaging

## Requirements

- Windows 11.
- Visual Studio 2022 with the **Desktop development with C++** workload.
- Windows 11 SDK containing build 26100 or later.
- CMake 3.24 or later.

The production build uses only Windows system libraries and statically links the Microsoft C/C++ runtime.

## Visual Studio build

From a Developer PowerShell:

```powershell
cmake --preset windows-vs2022
cmake --build --preset windows-debug --parallel
cmake --build --preset windows-release --parallel
```

Outputs are under:

```text
build/windows-vs2022/src/windows/Debug/
build/windows-vs2022/src/windows/Release/
```

## Portable ZIP

```powershell
./scripts/build-portable.ps1 -Configuration Release
```

The ZIP is written under `out/`. The portable app registers itself under the current user's `Run` key only when **Start at sign-in** is enabled.

## MSIX

Create an unsigned development package with:

```powershell
./scripts/build-msix.ps1 -Configuration Release
```

For a signed package, the manifest publisher must exactly match the signing certificate subject:

```powershell
$password = Read-Host -AsSecureString "Certificate password"
./scripts/build-msix.ps1 `
  -Configuration Release `
  -Publisher "CN=Your Publisher" `
  -CertificatePath "C:\secure\NetStatBar.pfx" `
  -CertificatePassword $password
```

Never commit a PFX, certificate password, or private key. Public distribution requires an Authenticode/MSIX certificate trusted by the target machines.

The packaged startup extension always invokes `NetStatBarStartup.exe`; that launcher honors the in-app opt-in stored by the main process. Users can additionally disable the package in Windows Startup Apps, and the application does not attempt to override that system choice.

## macOS core build and Windows cross-build

Portable logic can be tested on macOS:

```sh
cmake --preset default
cmake --build --preset default --parallel
ctest --preset default
```

With Homebrew MinGW-w64 installed, compile and link all x64 Windows sources:

```sh
cmake --preset mingw-cross
cmake --build --preset mingw-cross --parallel
```

The MinGW output is a compile-check artifact, not a release artifact. Public binaries must come from the MSVC Release build and pass the Windows runtime matrix.

## Version updates

Before a release, update these values together:

- `project(... VERSION ...)` in `CMakeLists.txt`.
- File/product versions in `resources/NetStatBar.rc`.
- Assembly version in `app.manifest`.
- Default package version in `packaging/AppxManifest.xml` and the release command.
