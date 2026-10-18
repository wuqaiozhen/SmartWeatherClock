@echo off
chcp 65001 >nul
set PYTHONIOENCODING=utf-8
set PORT=%~1
if "%PORT%"=="" set PORT=COM9
"D:\esp\Espressif\python_env\idf5.4_py3.11_env\Scripts\python.exe" -u "D:\esp\projects\weather\read_log.py" %PORT%
pause
