@echo off
setlocal
rem Run this after EVERY CubeMX "GENERATE CODE".
rem It fixes the while(1) brace that CubeMX swallows, and reports
rem Chinese comments that CubeMX mangled into garbage.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\post_gen_fix.ps1"
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\check_encoding.ps1"
echo.
pause
