@echo off
::============================================================================
:: Make sure we can use command shell extensions
::============================================================================
    VERIFY OTHER 2>nul
    SETLOCAL ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
    IF NOT ERRORLEVEL 1 goto ExtOk
        echo *
        echo * Error: Unable to enable DOS extensions
        echo *
        EXIT /B 1
:ExtOk

set ScriptPath=%~d0%~p0

::============================================================================
::Detect VS2017
::============================================================================
    set WHERE_EXE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

    if exist %WHERE_EXE% (
        for /f "usebackq tokens=1* delims=: " %%i in (`%WHERE_EXE% -latest -requires Microsoft.Component.MSBuild`) do (
          if /i "%%i"=="installationPath" set InstallDir=%%j
        )

        if exist "%InstallDir%\VC\Auxiliary\Build" (
            set VCDIR="%InstallDir%\VC\Auxiliary\Build\"
        )

        if defined VCDIR (
            echo "Visual Studio 2017 (version 15) detected"
            set VC_VER_STR=15 2017
            goto :VCOK
        )
    )

::============================================================================
::Detect VS14
::============================================================================
    set VC_KEY_NAME="HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\Setup\VC"
    set VC_KEY_VALUE="ProductDir"
    set VCDIR=""

    for /f "usebackq tokens=1-2,*" %%a in (`REG QUERY %VC_KEY_NAME% /v %VC_KEY_VALUE%`) do (
        set VCDIR="%%c"
    )

    if defined VCDIR (
        echo "Visual Studio 2015 (version 14) detected"
        set VC_VER_STR=14 2015
        goto :VCOK
    )

echo ERROR: Visual Studio 2015 or 2017 is required.

goto :EndOfScript

:VCOK
::============================================================================
::Run CMake
::============================================================================
    if not exist %ScriptPath%_build mkdir %ScriptPath%_build
    pushd %ScriptPath%_build
    cmake .. -G "Visual Studio %VC_VER_STR% Win64"
    cmake --build . --config Release
    ctest . -C Release -V
    popd

:EndOfScript
endlocal
goto :EOF

