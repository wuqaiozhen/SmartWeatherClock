@echo off
call "D:\esp\Espressif\frameworks\esp-idf-v5.4.4\export.bat"
if errorlevel 1 goto fail

cd /d "D:\esp\projects\weather"
idf.py build > build.log 2>&1
if errorlevel 1 goto fail

echo.
echo ==========================================
echo   [OK] BUILD SUCCESS
echo ==========================================
goto end

:fail
echo.
echo ==========================================
echo   [FAIL] BUILD FAILED
echo   Open build.log and search for "error:"
echo   Or tell the AI: read build.log
echo ==========================================

:end
echo.
pause
