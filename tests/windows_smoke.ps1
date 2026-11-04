$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class NativeSmoke {
    public delegate bool EnumWindow(IntPtr h, IntPtr data);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr parent, EnumWindow callback, IntPtr data);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindow callback, IntPtr data);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern IntPtr GetDlgItem(IntPtr h, int id);
    [DllImport("user32.dll")] public static extern IntPtr SendMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
}
'@
$binary = (Resolve-Path "$PSScriptRoot/../dist/ClassicFlipClock.scr").Path
$processes = [Collections.Generic.List[Diagnostics.Process]]::new()
function Launch([string] $arguments) {
    $info = [Diagnostics.ProcessStartInfo]::new($binary, $arguments)
    $info.UseShellExecute = $false
    $process = [Diagnostics.Process]::Start($info)
    $processes.Add($process)
    return $process
}
function Pump([int] $milliseconds) {
    $until = [DateTime]::UtcNow.AddMilliseconds($milliseconds)
    while ([DateTime]::UtcNow -lt $until) {
        [Windows.Forms.Application]::DoEvents()
        Start-Sleep -Milliseconds 25
    }
}
function WindowsFor([Diagnostics.Process] $process, [IntPtr] $parent = [IntPtr]::Zero) {
    $handles = [Collections.Generic.List[IntPtr]]::new()
    $callback = [NativeSmoke+EnumWindow] {
        param($h, $data)
        [uint32] $owner = 0
        [void][NativeSmoke]::GetWindowThreadProcessId($h, [ref] $owner)
        if ($owner -eq $process.Id) { $handles.Add($h) }
        return $true
    }
    if ($parent -eq [IntPtr]::Zero) { [void][NativeSmoke]::EnumWindows($callback, [IntPtr]::Zero) }
    else { [void][NativeSmoke]::EnumChildWindows($parent, $callback, [IntPtr]::Zero) }
    return $handles.ToArray()
}
$form = $null
$settingsPath = 'HKCU:\Software\ClassicFlipClock'
$hadSettingsKey = Test-Path $settingsPath
$previousFlags = Get-ItemProperty -LiteralPath $settingsPath -Name Flags -ErrorAction SilentlyContinue
try {
    foreach ($arguments in @('/p', '/p 0', '/p 12abc', '/unknown')) {
        $p = Launch $arguments
        if (-not $p.WaitForExit(5000) -or $p.ExitCode -ne 1) { throw "Invalid arguments did not fail closed: $arguments" }
    }
    $form = [Windows.Forms.Form]::new()
    $form.ClientSize = [Drawing.Size]::new(640, 360)
    $form.Show()
    $p = Launch ('/p ' + $form.Handle.ToInt64())
    Pump 1500
    if ($p.HasExited -or @(WindowsFor $p $form.Handle).Count -ne 1) { throw 'Preview was not embedded into the supplied parent.' }
    $form.ClientSize = [Drawing.Size]::new(320, 180)
    Pump 300
    $form.Close()
    $form.Dispose()
    $form = $null
    if (-not $p.WaitForExit(5000)) { throw 'Preview did not exit after its parent was destroyed.' }
    $p = Launch '/c'
    Pump 800
    $handles = @(WindowsFor $p)
    if ($p.HasExited -or $handles.Count -lt 1) { throw 'Configuration dialog did not open.' }
    $dialog = $handles | Where-Object { [NativeSmoke]::GetDlgItem($_, 1005) -ne [IntPtr]::Zero } | Select-Object -First 1
    if (-not $dialog) { throw 'Day-of-week checkbox is missing.' }
    foreach ($id in @(1003, 1005)) {
        [void][NativeSmoke]::SendMessage([NativeSmoke]::GetDlgItem($dialog, $id), 0x00F1, [IntPtr] 1, [IntPtr]::Zero)
    }
    [void][NativeSmoke]::PostMessage($dialog, 0x0111, [IntPtr] 1, [IntPtr]::Zero)
    if (-not $p.WaitForExit(5000)) { throw 'Configuration dialog did not save and close.' }
    $savedFlags = (Get-ItemProperty -LiteralPath $settingsPath -Name Flags).Flags
    if (($savedFlags -band 20) -ne 20) { throw 'Date and weekday options were not saved.' }
    $p = Launch '/c'
    Pump 800
    $dialog = @(WindowsFor $p) | Where-Object { [NativeSmoke]::GetDlgItem($_, 1005) -ne [IntPtr]::Zero } | Select-Object -First 1
    if (-not $dialog) { throw 'Settings did not reopen.' }
    foreach ($id in @(1003, 1005)) {
        $checked = [NativeSmoke]::SendMessage([NativeSmoke]::GetDlgItem($dialog, $id), 0x00F0, [IntPtr]::Zero, [IntPtr]::Zero)
        if ($checked.ToInt64() -ne 1) { throw "Saved checkbox $id did not reload." }
    }
    [void][NativeSmoke]::PostMessage($dialog, 0x0010, [IntPtr]::Zero, [IntPtr]::Zero)
    if (-not $p.WaitForExit(5000)) { throw 'Configuration dialog did not close.' }
    $p = Launch '/s'
    Pump 400
    $handles = @(WindowsFor $p)
    if ($p.HasExited -or $handles.Count -lt 1) { throw 'Fullscreen did not open.' }
    [void][NativeSmoke]::PostMessage($handles[0], 0x0100, [IntPtr] 27, [IntPtr]::Zero)
    if (-not $p.WaitForExit(5000)) { throw 'Keyboard input did not close fullscreen.' }
    Write-Host 'Windows smoke checks passed: invalid arguments, embedded preview, parent destruction, date/weekday settings persistence, fullscreen and keyboard exit.'
} finally {
    if ($form) { $form.Dispose() }
    foreach ($p in $processes) { if (-not $p.HasExited) { $p.Kill(); $p.WaitForExit() }; $p.Dispose() }
    if ($previousFlags) {
        Set-ItemProperty -LiteralPath $settingsPath -Name Flags -Value $previousFlags.Flags
    } elseif (Test-Path $settingsPath) {
        Remove-ItemProperty -LiteralPath $settingsPath -Name Flags -ErrorAction SilentlyContinue
        if (-not $hadSettingsKey) { Remove-Item -LiteralPath $settingsPath }
    }
}
