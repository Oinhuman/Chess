@echo off
setlocal

call "%~dp0build.bat"
if errorlevel 1 exit /b %errorlevel%

"%~dp01.exe"
