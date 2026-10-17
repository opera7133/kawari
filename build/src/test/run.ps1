<#
.SYNOPSIS
    Minimal tests for the built KAWARI binaries (kosui.exe and shiori.dll).
.DESCRIPTION
    Two suites, both driven by a plain text case file:

    kosui   Feeds one line to kosui.exe (echo mode or command mode) and compares the output with the
            expected text. This tests the dictionary / KIS engine. Every case runs in a fresh copy of
            test/kosui, so cases do not affect each other.
            kosui must be told to quit with "exit": at the end of its input it loops forever, so the
            runner always sends "exit" and kills kosui after -TimeoutSeconds.

    shiori  Builds test/harness/shiori_harness.c, loads shiori.dll with the ghost in test/shiori
            (load), sends the SHIORI/3.0 requests of test/shiori/cases.txt (request) and unloads it.
            Every expected text must be found in the matching response. This tests the SHIORI layer.
            The harness is built with -CC and must have the same bitness as shiori.dll (32bit).

    The binaries must already be built (build/src: gmake -f gcc.mak). Nothing is written to the
    repository: the work files go to a temporary folder.

    Exit codes: 0 = all passed, 1 = a case failed, 2 = the tests could not run (missing binaries,
    compiler problems, wrong bitness).
.PARAMETER BinDir
    Folder with kosui.exe and shiori.dll (default: build/mach/mingw).
.PARAMETER Suite
    all (default), kosui or shiori.
.PARAMETER CC
    C compiler for the harness (default: gcc). Put the 32bit MinGW first in PATH or give its full path.
.PARAMETER KeepWork
    Keep the temporary work folder and print its path.
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1
.EXAMPLE
    powershell -NoProfile -ExecutionPolicy Bypass -File build/src/test/run.ps1 -Suite kosui
#>
[CmdletBinding()]
param(
    [string]$BinDir,
    [ValidateSet('all', 'kosui', 'shiori')]
    [string]$Suite = 'all',
    [string]$CC = 'gcc',
    [int]$TimeoutSeconds = 30,
    [switch]$KeepWork
)
$ErrorActionPreference = 'Stop'

# Bytes are passed through unchanged (one byte = one char), so no character code conversion happens.
$Latin1 = [Text.Encoding]::GetEncoding(28591)

$script:Passed = 0
$script:Failed = 0

function Stop-Setup([string]$Message) {
    Write-Host "test: CANNOT RUN - $Message"
    exit 2
}

function Quote-Arg([string]$Text) {
    return '"' + $Text.TrimEnd('\') + '"'
}

# Runs a program, optionally feeding it text, and returns its output. Kills it after the timeout.
# The input goes through a file and "<" of cmd.exe: a redirected stdin of .NET Framework starts with a
# UTF-8 BOM, which breaks the first line that the program reads.
function Invoke-Program([string]$File, [string[]]$ArgList, [string]$WorkDir, [string]$InputText) {
    $psi = New-Object Diagnostics.ProcessStartInfo
    $psi.WorkingDirectory = $WorkDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.StandardOutputEncoding = $Latin1
    $psi.StandardErrorEncoding = $Latin1
    $inputFile = $null
    if ($InputText) {
        $inputFile = [IO.Path]::GetTempFileName()
        [IO.File]::WriteAllBytes($inputFile, $Latin1.GetBytes($InputText))
        $psi.FileName = $env:ComSpec
        $psi.Arguments = '/d /s /c ""' + $File + '" ' + ($ArgList -join ' ') + ' < "' + $inputFile + '""'
    } else {
        $psi.FileName = $File
        $psi.Arguments = ($ArgList -join ' ')
    }
    try {
        $proc = [Diagnostics.Process]::Start($psi)
        $outTask = $proc.StandardOutput.ReadToEndAsync()
        $errTask = $proc.StandardError.ReadToEndAsync()
        $timedOut = -not $proc.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) {
            # Kill the whole tree: cmd.exe is only the parent of the program.
            # It may have ended just now, so ignore a failure of taskkill.
            try { & taskkill /F /T /PID $proc.Id 2>&1 | Out-Null } catch { }
            $proc.WaitForExit()
        } else {
            $proc.WaitForExit()
        }
    } finally {
        if ($inputFile) { Remove-Item -LiteralPath $inputFile -Force -ErrorAction SilentlyContinue }
    }
    return @{
        ExitCode = $proc.ExitCode
        StdOut   = $outTask.Result
        StdErr   = $errTask.Result
        TimedOut = $timedOut
    }
}

# Reads a case file: a line that starts with $ExpectMark adds an expected line to the current case,
# any other line (not blank, not "#") starts a new case ("<verb> <rest>").
function Read-Cases([string]$Path, [string]$ExpectMark, [string[]]$Verbs) {
    $cases = New-Object System.Collections.Generic.List[object]
    $current = $null
    $lineNo = 0
    foreach ($line in [IO.File]::ReadAllLines($Path, $Latin1)) {
        $lineNo++
        if ($line.Trim() -eq '' -or $line.StartsWith('#')) { continue }
        if ($line.StartsWith($ExpectMark)) {
            if (-not $current) { Stop-Setup "${Path}:${lineNo}: expected text before any case" }
            $text = $line.Substring(1)
            if ($text.StartsWith(' ')) { $text = $text.Substring(1) }
            $current.Expected.Add($text)
            continue
        }
        $verb, $rest = $line -split '\s+', 2
        if ($Verbs -notcontains $verb) { Stop-Setup "${Path}:${lineNo}: unknown verb '$verb' (expected: $($Verbs -join ', '))" }
        $current = @{ Line = $lineNo; Verb = $verb; Rest = $rest; Expected = (New-Object System.Collections.Generic.List[string]) }
        $cases.Add($current)
    }
    foreach ($case in $cases) {
        if ($case.Expected.Count -eq 0) { Stop-Setup "${Path}:$($case.Line): the case has no expected text" }
    }
    return $cases
}

function Write-Result([bool]$Ok, [string]$Name, [string[]]$Detail) {
    if ($Ok) {
        $script:Passed++
        Write-Host "ok  $Name"
    } else {
        $script:Failed++
        Write-Host "NG  $Name"
        foreach ($d in $Detail) { Write-Host "      $d" }
    }
}

# Returns i386 or x64 from the PE header.
function Get-PeMachine([string]$Path) {
    $b = [IO.File]::ReadAllBytes($Path)
    $pe = [BitConverter]::ToInt32($b, 0x3C)
    $machine = [BitConverter]::ToUInt16($b, $pe + 4)
    if ($machine -eq 0x14c) { return 'i386' }
    if ($machine -eq 0x8664) { return 'x64' }
    return ('0x{0:x}' -f $machine)
}

function Show-Text([string]$Text) {
    return ($Text -replace "`r", '\r' -replace "`n", '\n')
}

# ---- kosui ----------------------------------------------------------------------------------

function Test-Kosui([string]$Work) {
    $exe = Join-Path $BinDir 'kosui.exe'
    if (-not (Test-Path -LiteralPath $exe)) { Stop-Setup "kosui.exe was not found in $BinDir" }
    $fixture = Join-Path $PSScriptRoot 'kosui'
    $cases = Read-Cases (Join-Path $fixture 'cases.txt') '=' @('echo', 'cmd')

    Write-Host "== kosui ($($cases.Count) cases)"
    $banner = "'.' is mode change command.`n"
    $index = 0
    foreach ($case in $cases) {
        $index++
        $name = "kosui:$($case.Line) $($case.Verb) $($case.Rest)"
        $dir = Join-Path $Work ('kosui' + $index)
        New-Item -ItemType Directory -Path $dir | Out-Null
        Copy-Item -Path (Join-Path $fixture 'kawarirc.kis') -Destination $dir

        if ($case.Verb -eq 'cmd') { $session = ".`n$($case.Rest)`nexit`n" } else { $session = "$($case.Rest)`nexit`n" }
        $run = Invoke-Program $exe @() $dir $session
        $out = $run.StdOut -replace "`r`n", "`n"
        $expected = $case.Expected -join "`n"

        if ($run.TimedOut) {
            Write-Result $false $name @("kosui did not finish within $TimeoutSeconds seconds")
            continue
        }
        $pos = $out.IndexOf($banner)
        if ($pos -lt 0) {
            $hint = ''
            if ($out.Contains('Attach :')) { $hint = ' (a running ghost was found and kosui asked which one to attach to; quit the ghost)' }
            Write-Result $false $name @("kosui did not reach its prompt$hint", "output: $(Show-Text $out)")
            continue
        }
        $rest = $out.Substring($pos + $banner.Length)
        $m = [regex]::Match($rest, '(?s)^(?:(?:echo|command)-mode > )+(.*?)\n(?:echo|command)-mode > $')
        if (-not $m.Success) {
            Write-Result $false $name @("could not find the result in the output", "output: $(Show-Text $rest)")
            continue
        }
        $actual = $m.Groups[1].Value
        Write-Result ($actual -ceq $expected) $name @("expected: $(Show-Text $expected)", "actual:   $(Show-Text $actual)")
    }
}

# ---- shiori ---------------------------------------------------------------------------------

function Test-Shiori([string]$Work) {
    $dll = Join-Path $BinDir 'shiori.dll'
    if (-not (Test-Path -LiteralPath $dll)) { Stop-Setup "shiori.dll was not found in $BinDir" }
    $fixture = Join-Path $PSScriptRoot 'shiori'
    $cases = Read-Cases (Join-Path $fixture 'cases.txt') '~' @('GET', 'NOTIFY')

    # The harness loads the DLL into its own process, so both must be the same bitness.
    $dllMachine = Get-PeMachine $dll
    try {
        $target = (Invoke-Program $CC @('-dumpmachine') $PSScriptRoot '').StdOut.Trim()
    } catch {
        Stop-Setup "cannot run the C compiler '$CC' (use -CC to give a 32bit MinGW gcc)"
    }
    $compilerMachine = 'other'
    if ($target -match '^(i[3-6]86)-') { $compilerMachine = 'i386' } elseif ($target -match '^x86_64-') { $compilerMachine = 'x64' }
    if ($dllMachine -ne $compilerMachine) {
        Stop-Setup "shiori.dll is $dllMachine but '$CC' builds for $target. Put a matching MinGW first in PATH or use -CC"
    }

    $harness = Join-Path $Work 'shiori_harness.exe'
    $build = Invoke-Program $CC @('-O1', '-Wall', '-o', (Quote-Arg $harness), (Quote-Arg (Join-Path $PSScriptRoot 'harness\shiori_harness.c'))) $Work ''
    if ($build.ExitCode -ne 0 -or -not (Test-Path -LiteralPath $harness)) {
        Stop-Setup "building the harness failed:`n$($build.StdErr)"
    }

    $ghost = Join-Path $Work 'ghost'
    New-Item -ItemType Directory -Path $ghost | Out-Null
    Copy-Item -Path (Join-Path $fixture 'kawarirc.kis') -Destination $ghost
    Copy-Item -Path $dll -Destination $ghost

    # One request file per case.
    $requests = New-Object System.Collections.Generic.List[string]
    $index = 0
    foreach ($case in $cases) {
        $index++
        $id, $refs = $case.Rest -split '\s+', 2
        $headers = New-Object System.Collections.Generic.List[string]
        $headers.Add("$($case.Verb) SHIORI/3.0")
        $headers.Add('Charset: Shift_JIS')
        $headers.Add('Sender: SSP')
        $headers.Add('SecurityLevel: local')
        $headers.Add("ID: $id")
        if ($refs) {
            $n = 0
            foreach ($ref in ($refs -split ',')) { $headers.Add("Reference${n}: $($ref.Trim())"); $n++ }
        }
        $file = Join-Path $Work ('req{0:D3}.txt' -f $index)
        [IO.File]::WriteAllBytes($file, $Latin1.GetBytes(($headers -join "`r`n") + "`r`n`r`n"))
        $requests.Add((Quote-Arg $file))
    }

    $outFile = Join-Path $Work 'responses.txt'
    $argList = @((Quote-Arg $dll), (Quote-Arg $ghost), (Quote-Arg $outFile)) + $requests
    # The harness loads the copy in the ghost folder, so shiori.dll itself is never touched.
    $argList[0] = Quote-Arg (Join-Path $ghost 'shiori.dll')
    $run = Invoke-Program $harness $argList $Work ''
    if ($run.TimedOut) { Stop-Setup "the harness did not finish within $TimeoutSeconds seconds" }
    if ($run.ExitCode -ne 0) { Stop-Setup "the harness failed (exit code $($run.ExitCode)): $($run.StdErr.Trim())" }

    $responses = $Latin1.GetString([IO.File]::ReadAllBytes($outFile)) -split "`n=====`n"
    # The text ends with a separator, so there is one empty piece after the last response.
    if ($responses.Count -ne $cases.Count + 1) {
        Stop-Setup "expected $($cases.Count) responses but the harness wrote $($responses.Count - 1)"
    }

    Write-Host "== shiori ($($cases.Count) cases)"
    $index = 0
    foreach ($case in $cases) {
        $response = $responses[$index]
        $index++
        $name = "shiori:$($case.Line) $($case.Verb) $($case.Rest)"
        $missing = @($case.Expected | Where-Object { -not $response.Contains($_) })
        $detail = @()
        foreach ($text in $missing) { $detail += "missing: $text" }
        if ($missing.Count -gt 0) { $detail += "response: $(Show-Text $response)" }
        Write-Result ($missing.Count -eq 0) $name $detail
    }
}

# ---- main -----------------------------------------------------------------------------------

if (-not $BinDir) { $BinDir = Join-Path $PSScriptRoot '..\..\mach\mingw' }
if (-not (Test-Path -LiteralPath $BinDir)) { Stop-Setup "BinDir does not exist: $BinDir (build first: gmake -f gcc.mak in build/src)" }
$BinDir = (Resolve-Path -LiteralPath $BinDir).Path

$work = Join-Path ([IO.Path]::GetTempPath()) ('kawari-test-' + [guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $work | Out-Null
try {
    if ($Suite -eq 'all' -or $Suite -eq 'kosui') { Test-Kosui $work }
    if ($Suite -eq 'all' -or $Suite -eq 'shiori') { Test-Shiori $work }
} finally {
    if ($KeepWork) {
        Write-Host "work folder kept: $work"
    } else {
        Remove-Item -LiteralPath $work -Recurse -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ''
Write-Host "test: $($script:Passed) passed, $($script:Failed) failed"
if ($script:Failed -gt 0) { exit 1 }
exit 0
