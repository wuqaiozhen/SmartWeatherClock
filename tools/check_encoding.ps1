# Check hand-written sources for broken UTF-8 (CubeMX mangles Chinese comments).
# ASCII only - safe to read with any PowerShell code page.
$root = Split-Path -Parent $PSScriptRoot
$fffd = [char]0xFFFD
$bad  = 0

$dirs = @('Core', 'Hardware', 'GUI', 'cmake')
foreach ($d in $dirs) {
    $p = Join-Path $root $d
    if (-not (Test-Path $p)) { continue }
    Get-ChildItem -Path $p -Recurse -Include *.c, *.h -File | ForEach-Object {
        $t = [System.IO.File]::ReadAllText($_.FullName, [System.Text.Encoding]::UTF8)
        $n = ([regex]::Matches($t, [regex]::Escape([string]$fffd))).Count
        if ($n -gt 0) {
            Write-Host ("BAD  x{0}  {1}" -f $n, $_.FullName)
            $bad = 1
        }
    }
}

if ($bad -eq 0) {
    Write-Host 'encoding OK'
} else {
    Write-Host ''
    Write-Host 'fix: run "git diff <file>" and restore the broken comment lines from HEAD'
}
exit $bad
