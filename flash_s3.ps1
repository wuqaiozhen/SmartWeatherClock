# 自动等待 ESP32-S3 进入下载模式 -> 擦除 MicroPython -> 烧录 ESP-IDF 固件
$ErrorActionPreference = 'Continue'
$py  = 'D:\esp\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe'
$b   = 'D:\esp\projects\weather\build'
$log = 'D:\esp\projects\weather\flash_s3.log'

function Log([string]$m){
    $line = (Get-Date).ToString('HH:mm:ss') + '  ' + $m
    Write-Host $line
    Add-Content -Path $log -Value $line -Encoding UTF8
}

"===== flash_s3 开始 $(Get-Date) =====" | Set-Content -Path $log -Encoding UTF8

$files = @(
    @{ addr = '0x0';     path = "$b\bootloader\bootloader.bin" },
    @{ addr = '0x8000';  path = "$b\partition_table\partition-table.bin" },
    @{ addr = '0x10000'; path = "$b\wifi_station.bin" }
)
foreach ($f in $files) {
    if (-not (Test-Path $f.path)) { Log ("缺少文件: " + $f.path); exit 1 }
}

Log '等待进入下载模式的 ESP32-S3 (最多 10 分钟)...'
$port = $null
$deadline = (Get-Date).AddSeconds(600)
while (-not $port -and (Get-Date) -lt $deadline) {
    $cands = [System.IO.Ports.SerialPort]::GetPortNames() | Where-Object { $_ -ne 'COM5' -and $_ -ne 'COM6' }
    foreach ($p in $cands) {
        $out = & $py -m esptool --chip esp32s3 --port $p --before default_reset --after no_reset chip_id 2>&1
        $txt = ($out -join "`n")
        if ($txt -match 'Chip is ESP32-S3') {
            $port = $p
            Log "检测到 ESP32-S3 @ $p"
            $txt | Add-Content -Path $log -Encoding UTF8
            break
        }
    }
    if (-not $port) { Start-Sleep -Milliseconds 800 }
}
if (-not $port) {
    Log '超时：未检测到 ESP32-S3。请按住 BOOT -> 点一下 RST -> 松开 BOOT。'
    exit 2
}

# 识别真实 flash 容量
$size = '16MB'
$fid = & $py -m esptool --chip esp32s3 --port $port --after no_reset flash_id 2>&1
$fidTxt = ($fid -join "`n")
$fidTxt | Add-Content -Path $log -Encoding UTF8
if ($fidTxt -match 'Detected flash size:\s*(\S+)') { $size = $Matches[1] }
Log "Flash 容量: $size"

Log '擦除整片 flash (清掉 MicroPython)...'
$er = & $py -m esptool --chip esp32s3 --port $port --after no_reset erase_flash 2>&1
($er -join "`n") | Add-Content -Path $log -Encoding UTF8
Log '擦除完成'

Log '写入 ESP-IDF 固件...'
$argList = @('-m','esptool','--chip','esp32s3','--port',$port,'--after','hard_reset',
             'write_flash','--flash_mode','dio','--flash_freq','80m','--flash_size',$size)
foreach ($f in $files) { $argList += $f.addr; $argList += $f.path }
$w = & $py @argList 2>&1
$wTxt = ($w -join "`n")
$wTxt | Add-Content -Path $log -Encoding UTF8

if ($wTxt -match 'Hash of data verified') { Log '烧录成功!' } else { Log '烧录可能失败，请看日志。' }
Log '===== 结束 ====='
