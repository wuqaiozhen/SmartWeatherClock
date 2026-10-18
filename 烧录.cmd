@echo off
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.4\export.bat"
if errorlevel 1 goto fail

cd /d "D:\esp\projects\weather"
idf.py -p COM9 flash
if errorlevel 1 goto fail

echo.
echo ==========================================
echo   [OK] FLASH SUCCESS
echo   Now open:  kanrizhi  (see below)
echo ==========================================
goto end

:fail
echo.
echo ==========================================
echo   [FAIL] FLASH FAILED
echo.
echo   Common causes:
echo    1. The log window is still open and locks COM9
echo       -^> Press Ctrl+C in that window, or close it
echo    2. Wrong port
echo       -^> Check Device Manager
echo    3. Board not in download mode
echo       -^> Hold BOOT, tap RST, release BOOT
echo ==========================================

:end
echo.
pause
