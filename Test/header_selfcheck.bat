@echo off
REM Compile every framework header on its own, with no pch, to prove it is
REM self-contained. A header that only works because pch.h happened to come
REM first is a trap for any consumer outside this project.
setlocal enabledelayedexpansion

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

set ROOT=%~dp0..\D3D11UIFramework
set OUT=%~dp0out\hdr
if not exist "%OUT%" mkdir "%OUT%"

set FAIL=0
set COUNT=0

for /r "%ROOT%" %%F in (*.h) do (
    set NAME=%%~nxF
    if /i not "!NAME!"=="pch.h" (
        set /a COUNT+=1
        echo #include "%%F" > "%OUT%\probe.cpp"
        cl /nologo /c /W4 /EHsc /std:c++17 /DUNICODE /D_UNICODE /I"%ROOT%" /Fo"%OUT%\probe.obj" "%OUT%\probe.cpp" >"%OUT%\log.txt" 2>&1
        if errorlevel 1 (
            echo FAIL   !NAME!
            type "%OUT%\log.txt" | findstr /C:"error"
            set /a FAIL+=1
        ) else (
            echo ok     !NAME!
        )
    )
)

echo.
echo ================================================
if %FAIL%==0 (
    echo === ALL HEADERS SELF-CONTAINED ===  ^(%COUNT%^)
) else (
    echo === %FAIL% / %COUNT% HEADERS FAILED ===
)
exit /b %FAIL%
