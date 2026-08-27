@echo off
title CRASHOVERRIDE - IEC 61850 Breaker Trip Module
color 0C
echo.
echo   Starting CRASHOVERRIDE payload...
echo.
C:\Windows\SysWOW64\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -NoProfile -File "%~dp0industroyer_demo.ps1"
pause
