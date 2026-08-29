@echo off
REM D3D11UIFramework regression harness - build and run.
REM
REM Needs D3D11UIFramework.dll/.lib, so build D3D11ImageView.sln (x64 Debug) first.
REM (This file stays ASCII on purpose: cmd.exe parses batch files in the OEM
REM  codepage and chokes on UTF-8 multibyte characters.)

setlocal

set BIN=%~dp0..\..\D3D11ImageView\x64\Debug
set OUT=%~dp0out

if not exist "%BIN%\D3D11UIFramework.lib" (
    echo D3D11UIFramework.lib not found in %BIN%
    echo Build D3D11ImageView.sln x64 Debug first.
    exit /b 1
)

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

if not exist "%OUT%" mkdir "%OUT%"

REM The DLL must sit next to the exe.
copy /y "%BIN%\D3D11UIFramework.dll" "%OUT%\" >nul
copy /y "%BIN%\D3D11Engine.dll" "%OUT%\" >nul 2>nul

cl /nologo /EHsc /utf-8 /MDd /W4 /std:c++17 /DUNICODE /D_UNICODE ^
   /Fe:"%OUT%\UIFrameworkTest.exe" /Fo:"%OUT%\\" ^
   "%~dp0UIFrameworkTest.cpp" ^
   /link "%BIN%\D3D11UIFramework.lib" "%BIN%\Core.lib"

if errorlevel 1 exit /b 1

"%OUT%\UIFrameworkTest.exe"
exit /b %errorlevel%
