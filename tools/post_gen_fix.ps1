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

# ---------- 1.5) 保底: defaultTask 栈必须 >= 512 words (2KB) ----------
# CubeMX 里改了也会被重新生成; 这里做最后一道保险
# 原因: defaultTask 里会调 LOGI -> printf, 一次要 400~500 字节栈
$dfltPat = '(defaultTask_attributes\s*=\s*\{(?s).*?\.stack_size\s*=\s*)(\d+)(\s*\*\s*4)'
$m = [regex]::Match($t, $dfltPat)
if ($m.Success) {
    $words = [int]$m.Groups[2].Value
    if ($words -lt 512) {
        $t = [regex]::Replace($t, $dfltPat, '${1}512${3}')
        [System.IO.File]::WriteAllText($main, $t, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host ("main.c : defaultTask stack {0} words -> 512 words (FIXED)" -f $words)
    } else {
        Write-Host ("main.c : defaultTask stack {0} words OK" -f $words)
    }
} else {
    Write-Host 'main.c : !!! defaultTask_attributes not found'
}

# ---------- 1.7) 把我们自己的注释重写成 ASCII ----------
# 这两处注释在 CubeMX 管理区里, 中文会被 GBK 读写搞成乱码,
# 所以统一用英文, 就不会再被破坏。
$normMain = @(
    @{ pat = '(?m)^[ \t]*app_report\(\);.*$';
       rep = '    app_report();            /* report heap/stack every 10s */' },
    @{ pat = '(?m)^[ \t]*osDelay\(10000\);.*$';
       rep = '    osDelay(10000);          /* 10000 ticks = 10s (TICK_RATE_HZ=1000) */' }
)
$tn = $t
foreach ($f in $normMain) { $tn = [regex]::Replace($tn, $f.pat, $f.rep) }
if ($tn -ne $t) {
    [System.IO.File]::WriteAllText($main, $tn, (New-Object System.Text.UTF8Encoding($false)))
    $t = $tn
    Write-Host 'main.c : our comments normalized to ASCII'
}

$freertos = Join-Path $root 'Core\Src\freertos.c'
if (Test-Path $freertos) {
    $ft = [System.IO.File]::ReadAllText($freertos, [System.Text.Encoding]::UTF8)
    $fo = $ft
    $ft = [regex]::Replace($ft, '(?m)^/\* uart\.c .*$', '/* send one byte (implemented in uart.c) */')
    $ft = [regex]::Replace($ft, '(?s)(void vApplicationStackOverflowHook\(xTaskHandle xTask, signed char \*pcTaskName\)\r?\n\{)\r?\n.*?\r?\n(\s*const char \*p;)', "`$1`r`n   /* stack overflow hook: the stack is ALREADY corrupted here.`r`n    * Do NOT call printf/log (printf itself needs 400+ bytes of stack).`r`n    * Only use __io_putchar or direct register writes. */`r`n`$2")
    if ($ft -ne $fo) {
        [System.IO.File]::WriteAllText($freertos, $ft, (New-Object System.Text.UTF8Encoding($false)))
        Write-Host 'freertos.c : our comments normalized to ASCII'
    }
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
