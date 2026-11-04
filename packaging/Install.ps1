$ErrorActionPreference = 'Stop'
try {
    if (-not [Environment]::Is64BitOperatingSystem) { throw 'This package requires 64-bit Windows.' }
    $source = Join-Path $PSScriptRoot 'ClassicFlipClock.scr'
    $folder = Join-Path $env:LOCALAPPDATA 'ClassicFlipClock'
    $target = Join-Path $folder 'ClassicFlipClock.scr'
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    if ([IO.Path]::GetFullPath($source) -ne [IO.Path]::GetFullPath($target)) {
        Copy-Item -LiteralPath $source -Destination $target -Force
    }
    Start-Process -FilePath "$env:WINDIR\System32\rundll32.exe" -ArgumentList ('desk.cpl,InstallScreenSaver "{0}"' -f $target)
    Write-Host 'Classic Flip Clock is ready. Choose your idle timeout in Windows Screen Saver Settings and click Apply.'
} catch {
    Write-Host ('Installation failed: ' + $_.Exception.Message) -ForegroundColor Red
    exit 1
}
