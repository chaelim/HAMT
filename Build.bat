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
::Detect VS14
::============================================================================
    set VC_KEY_NAME="HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\14.0\Setup\VC"
    set VC_KEY_VALUE="ProductDir"
    set VCDIR=""

    for /f "usebackq tokens=1-2,*" %%a in (`REG QUERY %VC_KEY_NAME% /v %VC_KEY_VALUE%`) do (
        set VCDIR="%%c"
    )

    if NOT %VCDIR% == "" (
        set VC_VER_STR=14 2015
        goto :VCOK
    )

::============================================================================
::Detect VS12
::============================================================================
    set VC_KEY_NAME="HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\VisualStudio\12.0\Setup\VC"
    set VC_KEY_VALUE="ProductDir"
    set VCDIR=""

    for /f "usebackq tokens=1-2,*" %%a in (`REG QUERY %VC_KEY_NAME% /v %VC_KEY_VALUE%`) do (
        set VCDIR="%%c"
    )

    if NOT %VCDIR% == "" (
        set VC_VER_STR=12 2013
        goto :VCOK
    )

echo ERROR: Visual Studio 2013 or 2015 is not found.

goto :EndOfScript

:VCOK
::============================================================================
::Run CMake
::============================================================================
    if not exist %ScriptPath%\_build mkdir %ScriptPath%\_build
    pushd %ScriptPath%\_build
    cmake .. -G "Visual Studio %VC_VER_STR% Win64"
    cmake --build . --config Release
    ctest . -C Release -V
    popd

:EndOfScript
endlocal
goto :EOF

