@echo off
REM Built-in icon preview - renders every icon to icon_preview.png.
REM
REM No D3D device needed: the icons are plain D2D primitives drawn into a
REM WIC bitmap render target.

setlocal

set OUT=%~dp0out

call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1

if not exist "%OUT%" mkdir "%OUT%"

cl /nologo /EHsc /utf-8 /MDd /W4 /std:c++17 /DUNICODE /D_UNICODE /DBUILD_D3D11_UI_FRAMEWORK_INTERFACE_DLL ^
   /Fe:"%OUT%\IconPreview.exe" /Fo:"%OUT%\\" ^
   "%~dp0IconPreview.cpp" "%~dp0..\D3D11UIFramework\Resource\UIIconRenderer.cpp"

if errorlevel 1 exit /b 1

pushd "%OUT%"
"%OUT%\IconPreview.exe"
popd
exit /b %errorlevel%
