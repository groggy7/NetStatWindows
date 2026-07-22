[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$OutputDirectory = "out"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$OutputRoot = Join-Path $ProjectRoot $OutputDirectory
$PortableDirectory = Join-Path $OutputRoot "NetStatBar-portable-x64"
$ZipPath = Join-Path $OutputRoot "NetStatBar-portable-x64.zip"

cmake --preset windows-vs2022
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
cmake --build --preset "windows-$($Configuration.ToLowerInvariant())"
if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }

if (Test-Path $PortableDirectory) {
    Remove-Item -Recurse -Force $PortableDirectory
}
New-Item -ItemType Directory -Force $PortableDirectory | Out-Null
Copy-Item (
    Join-Path $ProjectRoot "build/windows-vs2022/src/windows/$Configuration/NetStatBar.exe"
) $PortableDirectory
Copy-Item (Join-Path $ProjectRoot "README.md") $PortableDirectory
Copy-Item (Join-Path $ProjectRoot "docs/PRIVACY.md") $PortableDirectory

if (Test-Path $ZipPath) {
    Remove-Item -Force $ZipPath
}
Compress-Archive -Path (Join-Path $PortableDirectory "*") -DestinationPath $ZipPath
Write-Host "Created $ZipPath"
