<#
.SYNOPSIS
  Installs, updates or uninstalls Glimmer on Windows (x64) from GitHub Releases.

.DESCRIPTION
  Downloads the latest (or a chosen) release, verifies it against the
  release's SHA256SUMS, installs it per-user (no admin rights needed) to
  %LOCALAPPDATA%\Programs\Glimmer, adds a Start menu shortcut and an entry in
  Settings > Apps so it can be uninstalled normally.

  Quick install (PowerShell):
    irm https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.ps1 | iex

  With options:
    & ([scriptblock]::Create((irm https://raw.githubusercontent.com/sSseeth/Glimmer/main/scripts/install.ps1))) -Version v0.2.0

.PARAMETER Version
  Release tag to install, e.g. v0.2.0. Defaults to the latest release.

.PARAMETER From
  Install from a folder that already contains the release .zip and SHA256SUMS.

.PARAMETER Uninstall
  Remove Glimmer. Your diary (in %APPDATA%\glimmer) is kept.

.PARAMETER AddToPath
  Also add the install folder to your user PATH.
#>
[CmdletBinding()]
param(
  [string]$Version = 'latest',
  [string]$From = '',
  [switch]$Uninstall,
  [switch]$AddToPath
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'  # Invoke-WebRequest is much faster without it

$Repo = 'sSseeth/Glimmer'
$InstallDir = Join-Path $env:LOCALAPPDATA 'Programs\Glimmer'
$Shortcut = Join-Path ([Environment]::GetFolderPath('Programs')) 'Glimmer.lnk'
$UninstallKey = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Glimmer'
$DataDir = Join-Path $env:APPDATA 'glimmer'
$AssetSuffix = '-windows-x64.zip'

function Say([string]$Message) { Write-Host "==> $Message" -ForegroundColor Cyan }

function Remove-FromUserPath([string]$Dir) {
  $path = [Environment]::GetEnvironmentVariable('Path', 'User')
  if (-not $path) { return }
  $kept = ($path -split ';') | Where-Object { $_ -and ($_.TrimEnd('\') -ne $Dir.TrimEnd('\')) }
  [Environment]::SetEnvironmentVariable('Path', ($kept -join ';'), 'User')
}

function Assert-NotRunning {
  $running = Get-Process -Name 'glimmer' -ErrorAction SilentlyContinue |
    Where-Object { $_.Path -and $_.Path.StartsWith($InstallDir, [StringComparison]::OrdinalIgnoreCase) }
  if ($running) { throw 'Glimmer is running. Close it (save your entry first) and run this again.' }
}

# --- Uninstall ---------------------------------------------------------------

if ($Uninstall) {
  Say 'Removing Glimmer'
  Assert-NotRunning
  if (Test-Path $InstallDir) { Remove-Item -Recurse -Force $InstallDir }
  if (Test-Path $Shortcut) { Remove-Item -Force $Shortcut }
  if (Test-Path $UninstallKey) { Remove-Item -Recurse -Force $UninstallKey }
  Remove-FromUserPath $InstallDir
  Say "Done. Your diary was NOT deleted; it's in: $DataDir"
  return
}

if (-not [Environment]::Is64BitOperatingSystem) { throw 'Glimmer needs 64-bit Windows.' }

# --- Locate and download the release -------------------------------------------

$work = Join-Path ([IO.Path]::GetTempPath()) ("glimmer-install-" + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $work | Out-Null

try {
  if ($From) {
    $zip = Get-ChildItem -Path $From -Filter "glimmer-*$AssetSuffix" | Sort-Object Name | Select-Object -Last 1
    if (-not $zip) { throw "No glimmer-*$AssetSuffix found in $From" }
    $zipPath = $zip.FullName
    $sumsPath = Join-Path $From 'SHA256SUMS'
    $tag = $zip.Name.Substring('glimmer-'.Length, $zip.Name.Length - 'glimmer-'.Length - $AssetSuffix.Length)
  }
  else {
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    if ($Version -eq 'latest') { $url = "https://api.github.com/repos/$Repo/releases/latest" }
    else { $url = "https://api.github.com/repos/$Repo/releases/tags/$Version" }
    Say "Looking up $Version release of $Repo"
    try { $release = Invoke-RestMethod -Uri $url -Headers @{ 'User-Agent' = 'glimmer-installer' } }
    catch { throw "Couldn't find release '$Version' (has one been published yet?)" }

    $tag = $release.tag_name
    $asset = $release.assets | Where-Object { $_.name -like "*$AssetSuffix" } | Select-Object -First 1
    $sums = $release.assets | Where-Object { $_.name -eq 'SHA256SUMS' } | Select-Object -First 1
    if (-not $asset) { throw "Release $tag has no Windows download." }
    if (-not $sums) { throw "Release $tag has no SHA256SUMS file." }

    $zipPath = Join-Path $work $asset.name
    $sumsPath = Join-Path $work 'SHA256SUMS'
    Say "Downloading Glimmer $tag"
    Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath -UseBasicParsing
    Invoke-WebRequest -Uri $sums.browser_download_url -OutFile $sumsPath -UseBasicParsing
  }

  # --- Verify ----------------------------------------------------------------
  if (-not (Test-Path $sumsPath)) { throw 'SHA256SUMS not found; refusing to install an unverified download.' }
  $zipName = Split-Path $zipPath -Leaf
  $line = Get-Content $sumsPath | Where-Object { ($_ -split '\s+\*?')[-1] -eq $zipName } | Select-Object -First 1
  if (-not $line) { throw "$zipName is not listed in SHA256SUMS." }
  $expected = ($line -split '\s+')[0].ToLowerInvariant()
  $actual = (Get-FileHash -Algorithm SHA256 -Path $zipPath).Hash.ToLowerInvariant()
  if ($expected -ne $actual) { throw "Checksum mismatch for $zipName (expected $expected, got $actual)." }
  Say 'Checksum OK'

  # --- Install ---------------------------------------------------------------
  Assert-NotRunning
  $unpacked = Join-Path $work 'unpacked'
  Expand-Archive -Path $zipPath -DestinationPath $unpacked -Force
  $source = Join-Path $unpacked 'glimmer'
  if (-not (Test-Path (Join-Path $source 'glimmer.exe'))) { throw 'The download does not contain glimmer.exe.' }

  Say "Installing to $InstallDir"
  if (Test-Path $InstallDir) { Remove-Item -Recurse -Force $InstallDir }
  New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
  Copy-Item -Path (Join-Path $source '*') -Destination $InstallDir -Recurse -Force
  $exe = Join-Path $InstallDir 'glimmer.exe'

  # Start menu shortcut
  $shell = New-Object -ComObject WScript.Shell
  $link = $shell.CreateShortcut($Shortcut)
  $link.TargetPath = $exe
  $link.WorkingDirectory = $InstallDir
  $link.IconLocation = "$exe,0"
  $link.Description = 'A small, private desktop diary'
  $link.Save()

  # Settings > Apps entry, so it can be uninstalled like any other app.
  $uninstaller = Join-Path $InstallDir 'install.ps1'
  New-Item -Path $UninstallKey -Force | Out-Null
  $props = @{
    DisplayName     = 'Glimmer'
    DisplayVersion  = $tag.TrimStart('v')
    Publisher       = 'Glimmer contributors'
    InstallLocation = $InstallDir
    DisplayIcon     = $exe
    URLInfoAbout    = "https://github.com/$Repo"
    UninstallString = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$uninstaller`" -Uninstall"
  }
  foreach ($name in $props.Keys) { New-ItemProperty -Path $UninstallKey -Name $name -Value $props[$name] -PropertyType String -Force | Out-Null }
  New-ItemProperty -Path $UninstallKey -Name 'NoModify' -Value 1 -PropertyType DWord -Force | Out-Null
  New-ItemProperty -Path $UninstallKey -Name 'NoRepair' -Value 1 -PropertyType DWord -Force | Out-Null

  if ($AddToPath) {
    Remove-FromUserPath $InstallDir
    $path = [Environment]::GetEnvironmentVariable('Path', 'User')
    if ($path) { $path = "$path;$InstallDir" } else { $path = $InstallDir }
    [Environment]::SetEnvironmentVariable('Path', $path, 'User')
  }

  Say "Installed Glimmer $tag"
  Write-Host '    Launch it from the Start menu (search "Glimmer").'
  Write-Host "    Uninstall from Settings > Apps, or run this script with -Uninstall."
  Write-Host "    Your diary will be stored in: $DataDir"
}
finally {
  Remove-Item -Recurse -Force $work -ErrorAction SilentlyContinue
}
