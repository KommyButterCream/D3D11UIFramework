@echo off
REM D3D11UIFramework regression harness - build and run.
REM
REM (This file stays ASCII on purpose: cmd.exe parses batch files in the OEM
REM  codepage and chokes on UTF-8 multibyte characters.)

setlocal

set SLN=%~dp0..\..\D3D11ImageView\D3D11ImageView.sln
set BIN=%~dp0..\..\D3D11ImageView\x64\Debug
set OUT=%~dp0out

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

REM Rebuild the library first.
REM
REM Without this the harness links whatever .lib happens to be lying around.
REM Editing the framework and running only this script then tests the OLD
REM binary, and a fix that already works looks like it failed. That happened.
echo Building solution (x64 Debug)...
msbuild "%SLN%" -p:Configuration=Debug -p:Platform=x64 -m -v:quiet -nologo
if errorlevel 1 (
    echo Library build FAILED.
    exit /b 1
)

if not exist "%BIN%\D3D11UIFramework.lib" (
    echo D3D11UIFramework.lib not found in %BIN%
    exit /b 1
)

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
