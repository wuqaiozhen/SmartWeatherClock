@echo off
setlocal
rem Run after EVERY CubeMX "GENERATE CODE":
rem CubeMX rewrites main.c / stm32f4xx_it.c in system ANSI(GBK) and
rem mangles Chinese comments. This script reports the damaged lines.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\check_encoding.ps1"
echo.
pause
