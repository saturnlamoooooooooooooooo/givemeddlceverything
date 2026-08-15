@echo off
setlocal

rem cd into vswhere's folder rather than quoting its full path inside the for:
rem cmd strips the outer quote pair off the command, which splits the path at
rem "Program Files", and the "(x86)" closes the for block early on top of that.
set "VSINSTALLER=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer"
if not exist "%VSINSTALLER%\vswhere.exe" (
    echo Couldn't find vswhere.exe - is Visual Studio installed?
    exit /b 1
)

set "VSPATH="
pushd "%VSINSTALLER%"
rem ".\" is required: this machine doesn't resolve executables from the cwd.
for /f "usebackq tokens=*" %%i in (`.\vswhere.exe -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
popd

if not defined VSPATH (
    echo Couldn't find a Visual Studio install with the C++ toolset.
    exit /b 1
)

echo Using %VSPATH%
rem vcvars chatters on stderr about its own vswhere lookup; msbuild failing is
rem what we actually check.
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul 2>nul
rem Found rather than hardcoded, so renaming the solution doesn't break this.
set "SLN="
for %%f in ("%~dp0*.sln") do set "SLN=%%f"
if not defined SLN (
    echo No .sln found in %~dp0
    exit /b 1
)

msbuild "%SLN%" /p:Configuration=Release /p:Platform=x64 /v:minimal /nologo
if errorlevel 1 exit /b 1

echo.
echo Built to %~dp0bin
