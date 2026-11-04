$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force build, dist | Out-Null
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    throw 'Run this script from the x64 Native Tools Command Prompt for Visual Studio (C++ desktop build tools).'
}
Push-Location src
try {
    & rc.exe /nologo /fo ../build/resource.res app.rc
    if ($LASTEXITCODE -ne 0) { throw 'Resource compilation failed.' }
} finally { Pop-Location }
& cl.exe /nologo /std:c++17 /utf-8 /EHsc /O2 /W4 /MT src/main.cpp /Fo:build/main.obj /Fe:dist/ClassicFlipClock.scr build/resource.res /link /SUBSYSTEM:WINDOWS /DYNAMICBASE /NXCOMPAT gdiplus.lib gdi32.lib user32.lib advapi32.lib
if ($LASTEXITCODE -ne 0) { throw 'C++ compilation failed.' }
Write-Host 'Built dist/ClassicFlipClock.scr'
