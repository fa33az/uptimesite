@echo off
setlocal enabledelayedexpansion

echo [+] Locating Visual Studio Build Tools...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "!VSWHERE!" (
    set "VSWHERE=C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
)

if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -products * -latest -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if not defined VS_PATH (
    echo [-] Visual Studio not detected by vswhere.exe. Trying fallback...
    set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
)

set "DEV_CMD=!VS_PATH!\Common7\Tools\VsDevCmd.bat"

if not exist "!DEV_CMD!" (
    echo [-] Error: Developer Command Prompt not found at !DEV_CMD!
    exit /b 1
)

echo [+] Initializing environment with !DEV_CMD!...
call "!DEV_CMD!" > nul

echo [+] Compiling main.cpp...
cl.exe /EHsc /O2 /std:c++17 /Fe:uptimesite.exe main.cpp

if !ERRORLEVEL! neq 0 (
    echo [-] Compilation failed!
    exit /b 1
)

echo [+] Compilation successful! Generated uptimesite.exe
