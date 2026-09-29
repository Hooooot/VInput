@echo off
setlocal
cd /d "%~dp0"

set CONFIG=Release
set PLATFORM=x64
set SOLUTION=VInput.slnx

where msbuild >nul 2>nul
if errorlevel 1 (
    goto :find_msbuild
) else (
    goto :ms_build
)

:find_msbuild
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
    echo vswhere not found. Install Visual Studio.
    goto :fail
)

for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set MSBUILD=%%i

if not defined MSBUILD (
    echo MSBuild not found.
    goto :fail
)

:ms_build
"%MSBUILD%" "%SOLUTION%" /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /m /v:minimal /nologo

if errorlevel 1 (
    echo Build failed.
    goto :fail
)

set DST=.\x64dll\
if not exist "%DST%" mkdir "%DST%"


copy /Y ".\x64\%CONFIG%\VInput.dll"        "%DST%"
copy /Y ".\VInput\Api.h"                   "%DST%VInput.h"
powershell -ExecutionPolicy Bypass -File ".\gen_vinput_py.ps1" ^
    -ApiH ".\VInput\Api.h" ^
    -OutPy "%DST%VInput.py" ^
    -VersionHeader ".\VInput\Version.h"
if errorlevel 1 (
    echo Copy failed.
    goto :fail
)

if errorlevel 1 goto :fail
if errorlevel 1 goto :fail
echo Build succeeded.
pause
exit /b 0

:fail
pause
exit /b 1