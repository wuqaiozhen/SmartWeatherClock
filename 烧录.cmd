@echo off
setlocal
set PORT=COM9

REM ===============================================================
REM  Step 1: release the serial port
REM    kill any python / esptool / openocd that may hold COM9
REM    (match by port name OR by known monitor scripts)
REM ===============================================================
echo [1/3] Releasing %PORT% ...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$me=$PID; Get-CimInstance Win32_Process | Where-Object { $_.ProcessId -ne $me -and $_.CommandLine -and $_.Name -match 'python|esptool|openocd|xtensa' -and ($_.CommandLine -match 'COM9|read_log|idf_monitor|esptool') } | ForEach-Object { Write-Host ('      kill PID ' + $_.ProcessId + '  ' + $_.Name); Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"

REM --- wait until the port can really be opened (max ~3s) ---
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ok=$false; for($i=0;$i -lt 10;$i++){ try{ $sp=New-Object System.IO.Ports.SerialPort('COM9'); $sp.Open(); $sp.Close(); $ok=$true; break } catch { Start-Sleep -Milliseconds 300 } }; if($ok){ Write-Host '      port is free' } else { Write-Host '      [WARN] port still busy - flash may fail' }"
timeout /t 1 /nobreak >nul

REM ===============================================================
REM  Step 2: build + flash
REM ===============================================================
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.4\export.bat"
if errorlevel 1 goto fail

cd /d "D:\esp\projects\weather"
echo.
echo [2/3] Flashing %PORT% ...
idf.py -p %PORT% flash
if errorlevel 1 goto fail

REM ===============================================================
REM  Step 3: done
REM ===============================================================
echo.
echo ==========================================
echo   [OK] FLASH SUCCESS
echo   (this window closes in about 3 seconds)
echo ==========================================
ping -n 4 127.0.0.1 >nul
exit /b 0

:fail
echo.
echo ==========================================
echo   [FAIL] FLASH FAILED
echo.
echo   Common causes:
echo    1. Port still busy
echo       -^> close every log window (press Ctrl+C inside it,
echo          do not click the X button)
echo    2. Wrong port
echo       -^> check Device Manager
echo    3. Board stuck
echo       -^> hold BOOT, tap RST, release BOOT
echo    4. Not in the ESP-IDF env
echo       -^> run 编译.cmd once to verify
echo ==========================================
pause
exit /b 1
