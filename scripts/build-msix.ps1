[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$Version = "0.1.0.0",
    [string]$Publisher = "CN=NetStatBar Development",
    [string]$OutputDirectory = "out",
    [string]$CertificatePath = "",
    [securestring]$CertificatePassword,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$BuildDirectory = Join-Path $ProjectRoot "build/windows-vs2022"
$OutputRoot = Join-Path $ProjectRoot $OutputDirectory
$StageDirectory = Join-Path $OutputRoot "msix-stage"
$PackagePath = Join-Path $OutputRoot "NetStatBar-$Version-x64.msix"

function Find-WindowsSdkTool([string]$Name) {
    $Command = Get-Command $Name -ErrorAction SilentlyContinue
    if ($Command) {
        return $Command.Source
    }

    $SdkRoot = Join-Path ${env:ProgramFiles(x86)} "Windows Kits/10/bin"
    $Candidate = Get-ChildItem $SdkRoot -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        ForEach-Object { Join-Path $_.FullName "x64/$Name" } |
        Where-Object { Test-Path $_ } |
        Select-Object -First 1
    if (-not $Candidate) {
        throw "$Name was not found. Install the Windows SDK or use a Developer PowerShell."
    }
    return $Candidate
}

if (-not $SkipBuild) {
    cmake --preset windows-vs2022
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed." }
    cmake --build --preset "windows-$($Configuration.ToLowerInvariant())"
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed." }
}

New-Item -ItemType Directory -Force $OutputRoot | Out-Null
if (Test-Path $StageDirectory) {
    Remove-Item -Recurse -Force $StageDirectory
}
New-Item -ItemType Directory -Force $StageDirectory | Out-Null
New-Item -ItemType Directory -Force (Join-Path $StageDirectory "Assets") | Out-Null

$BinaryDirectory = Join-Path $BuildDirectory "src/windows/$Configuration"
Copy-Item (Join-Path $BinaryDirectory "NetStatBar.exe") $StageDirectory
Copy-Item (Join-Path $BinaryDirectory "NetStatBarStartup.exe") $StageDirectory
Copy-Item (Join-Path $ProjectRoot "packaging/assets/*") (Join-Path $StageDirectory "Assets")

$Manifest = Get-Content (Join-Path $ProjectRoot "packaging/AppxManifest.xml") -Raw
$Manifest = $Manifest.Replace(
    'Version="0.1.0.0"',
    ('Version="{0}"' -f $Version))
$Manifest = $Manifest.Replace(
    'Publisher="CN=NetStatBar Development"',
    ('Publisher="{0}"' -f $Publisher))
Set-Content -Path (Join-Path $StageDirectory "AppxManifest.xml") -Value $Manifest -Encoding utf8

if (Test-Path $PackagePath) {
    Remove-Item -Force $PackagePath
}
$MakeAppx = Find-WindowsSdkTool "makeappx.exe"
$MakeAppxOutput = & $MakeAppx pack /d $StageDirectory /p $PackagePath /o 2>&1
$MakeAppxExitCode = $LASTEXITCODE
$MakeAppxOutput | ForEach-Object { Write-Host $_ }
if ($MakeAppxExitCode -ne 0) {
    $Details = ($MakeAppxOutput | Select-Object -Last 10) -join " | "
    throw "MakeAppx failed with exit code ${MakeAppxExitCode}: $Details"
}

if ($CertificatePath) {
    $SignTool = Find-WindowsSdkTool "signtool.exe"
    $PasswordArguments = @()
    if ($CertificatePassword) {
        $PlainPassword = [System.Net.NetworkCredential]::new(
            "", $CertificatePassword).Password
        $PasswordArguments = @("/p", $PlainPassword)
    }
    & $SignTool sign /fd SHA256 /f $CertificatePath @PasswordArguments $PackagePath
    if ($LASTEXITCODE -ne 0) { throw "SignTool failed." }
}

Write-Host "Created $PackagePath"
