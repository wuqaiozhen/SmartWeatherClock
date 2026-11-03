# ============================================================
#  每次 CubeMX "GENERATE CODE" 之后跑一次
#  (这个脚本是幂等的: 重复跑多少次结果都一样)
#
#  修两件 CubeMX 老是搞坏的事:
#   1) main.c 里 while(1) 的右括号被吞掉, 或多余一个
#      表现为编译报 "nested functions" / "expected ... at end of input"
#      / "expected identifier or '(' before '}' token"
#   2) 检查中文注释有没有被破坏成乱码 (这个必须人工改回)
# ============================================================

$root = Split-Path -Parent $PSScriptRoot
$main = Join-Path $root 'Core\Src\main.c'

if (-not (Test-Path $main)) { Write-Host "main.c not found"; exit 1 }

# ---------- 1) 把 "USER CODE BEGIN 3" 到下一个 '/**' 之间, 整体重写成标准布局 ----------
$t = [System.IO.File]::ReadAllText($main, [System.Text.Encoding]::UTF8)

$pat = [regex]::Escape('/* USER CODE BEGIN 3 */') + '(?s).*?(?=\r?\n/\*\*)'

$rep = "/* USER CODE BEGIN 3 */`r`n" +
       "    /* biz logic moved to Hardware/app.c (clock/ui/comm/sensor tasks) */`r`n" +
       "    /* USER CODE END 3 */`r`n" +
       "  }`r`n" +
       "}"

if ([regex]::IsMatch($t, $pat)) {
    $t2 = [regex]::Replace($t, $pat, $rep, 1)
    if ($t2 -ne $t) {
        [System.IO.File]::WriteAllText($main, $t2, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host 'main.c : USER CODE 3 block NORMALIZED'
    } else {
        Write-Host 'main.c : USER CODE 3 block already ok'
    }
} else {
    Write-Host 'main.c : !!! pattern not found, check manually'
}

# ---------- 2) 检查中文注释有没有被破坏 ----------
$fffd = [char]0xFFFD
$bad = 0
foreach ($d in @('Core', 'Hardware', 'GUI')) {
    $p = Join-Path $root $d
    if (-not (Test-Path $p)) { continue }
    Get-ChildItem -Path $p -Recurse -Include *.c, *.h -File | ForEach-Object {
        $s = [System.IO.File]::ReadAllText($_.FullName, [System.Text.Encoding]::UTF8)
        $n = ([regex]::Matches($s, [regex]::Escape([string]$fffd))).Count
        if ($n -gt 0) {
            Write-Host ("encoding BAD x{0}  {1}" -f $n, $_.FullName)
            $bad = 1
        }
    }
}
if ($bad -eq 0) { Write-Host 'encoding OK' } else { Write-Host '-> fix those comments by hand (git diff <file>)' }

exit 0
