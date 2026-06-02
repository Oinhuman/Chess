@echo off
setlocal

set "ROOT=%~dp0"
set "EXE=%ROOT%1.exe"

g++ "%ROOT%1.cpp" -o "%EXE%" -leasyx -lgdi32 -limm32 -lmsimg32 -lole32 -loleaut32 -lwinhttp -lcrypt32 -finput-charset=UTF-8 -fexec-charset=UTF-8
if errorlevel 1 exit /b %errorlevel%

echo Built "%EXE%"
