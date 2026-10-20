@echo off
REM ---------------------------------------------------------------
REM  1) Kill any running log reader (read_log.py) that locks COM9
REM ---------------------------------------------------------------
powershell -NoProfile -ExecutionPolicy Bypass -Command "Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -like '*read_log*' } | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }"
timeout /t 1 /nobreak >nul

call "D:\esp\Espressif\frameworks\esp-idf-v5.4.4\export.bat"
if errorlevel 1 goto fail

cd /d "D:\esp\projects\weather"
idf.py -p COM9 flash
if errorlevel 1 goto fail

echo.
echo ==========================================
echo   [OK] FLASH SUCCESS
echo ==========================================
goto end

:fail
echo.
echo ==========================================
echo   [FAIL] FLASH FAILED
echo.
echo   Common causes:
echo    1. Port still busy (close all log windows)
echo    2. Wrong port (check Device Manager)
echo    3. Board not in download mode
echo       -^> Hold BOOT, tap RST, release BOOT
echo ==========================================

:end
echo.
pause
