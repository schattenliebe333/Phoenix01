@echo off
REM ═══════════════════════════════════════════════════════════════════════════════
REM RAEL Installation Script (Windows)
REM Installs RAEL CLI to system PATH
REM Run as Administrator for system-wide installation
REM ═══════════════════════════════════════════════════════════════════════════════

setlocal enabledelayedexpansion

echo.
echo ╔═══════════════════════════════════════════════════════════════════╗
echo ║  RAEL INSTALLATION (Windows)                                      ║
echo ╚═══════════════════════════════════════════════════════════════════╝
echo.

REM Get script directory
set "SCRIPT_DIR=%~dp0"
set "ROOT_DIR=%SCRIPT_DIR%.."
set "BIN_DIR=%ROOT_DIR%\bin"

REM Check if binary exists
if not exist "%BIN_DIR%\rael_chat.exe" (
    echo [BUILD] rael_chat.exe nicht gefunden, kompiliere...

    REM Check for MSVC
    where cl >nul 2>nul
    if %errorlevel% neq 0 (
        echo [ERROR] Visual Studio C++ Compiler nicht gefunden.
        echo         Bitte Visual Studio Developer Command Prompt verwenden.
        echo         Oder führe zuerst Build.bat aus.
        pause
        exit /b 1
    )

    REM Build with MSVC
    cl /nologo /EHsc /std:c++17 /O2 ^
        /I"%ROOT_DIR%\include" ^
        "%ROOT_DIR%\src\cli\rael_chat.cpp" ^
        "%ROOT_DIR%\src\core\raelcore.cpp" ^
        "%ROOT_DIR%\src\core\ichbin.cpp" ^
        "%ROOT_DIR%\src\core\ethics.cpp" ^
        "%ROOT_DIR%\src\core\mathcore.cpp" ^
        "%ROOT_DIR%\src\core\semantic.cpp" ^
        "%ROOT_DIR%\src\core\resonance.cpp" ^
        "%ROOT_DIR%\src\core\telemetry.cpp" ^
        "%ROOT_DIR%\src\core\util.cpp" ^
        "%ROOT_DIR%\src\core\module_manager.cpp" ^
        "%ROOT_DIR%\src\core\events.cpp" ^
        "%ROOT_DIR%\src\core\metrics.cpp" ^
        "%ROOT_DIR%\src\core\star8.cpp" ^
        "%ROOT_DIR%\src\core\lane_scheduler.cpp" ^
        /Fe:"%BIN_DIR%\rael_chat.exe" ^
        /link /SUBSYSTEM:CONSOLE

    if %errorlevel% neq 0 (
        echo [ERROR] Build fehlgeschlagen.
        pause
        exit /b 1
    )
    echo [OK] rael_chat.exe kompiliert
)

REM Determine installation directory
set "INSTALL_DIR=%USERPROFILE%\.rael\bin"

REM Check for admin rights for system-wide installation
net session >nul 2>&1
if %errorlevel% equ 0 (
    echo [INFO] Administrator-Rechte erkannt
    set "INSTALL_DIR=C:\Program Files\RAEL"
    set "SYSTEM_INSTALL=1"
) else (
    echo [INFO] Benutzer-Installation nach %INSTALL_DIR%
    set "SYSTEM_INSTALL=0"
)

REM Create installation directory
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
)

REM Copy binary
echo [COPY] Kopiere rael_chat.exe nach %INSTALL_DIR%\rael.exe
copy /Y "%BIN_DIR%\rael_chat.exe" "%INSTALL_DIR%\rael.exe" >nul

REM Copy as rael_chat.exe as well
copy /Y "%BIN_DIR%\rael_chat.exe" "%INSTALL_DIR%\rael_chat.exe" >nul

REM Update PATH
echo [PATH] Aktualisiere PATH...

if "%SYSTEM_INSTALL%"=="1" (
    REM System PATH (requires admin)
    for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "SYSPATH=%%b"
    echo !SYSPATH! | find /i "%INSTALL_DIR%" >nul
    if errorlevel 1 (
        setx /M PATH "!SYSPATH!;%INSTALL_DIR%" >nul 2>&1
        echo [OK] System-PATH aktualisiert
    ) else (
        echo [OK] %INSTALL_DIR% ist bereits in System-PATH
    )
) else (
    REM User PATH
    for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v Path 2^>nul') do set "USERPATH=%%b"
    if "!USERPATH!"=="" set "USERPATH="

    echo !USERPATH! | find /i "%INSTALL_DIR%" >nul
    if errorlevel 1 (
        if "!USERPATH!"=="" (
            setx PATH "%INSTALL_DIR%" >nul 2>&1
        ) else (
            setx PATH "!USERPATH!;%INSTALL_DIR%" >nul 2>&1
        )
        echo [OK] Benutzer-PATH aktualisiert
        echo [INFO] Öffne ein neues Terminal, um 'rael' zu verwenden
    ) else (
        echo [OK] %INSTALL_DIR% ist bereits in PATH
    )
)

REM Verify installation
echo.
echo ╔═══════════════════════════════════════════════════════════════════╗
echo ║  INSTALLATION ERFOLGREICH!                                        ║
echo ╚═══════════════════════════════════════════════════════════════════╝
echo.
echo   Starte RAEL mit: rael
echo   Hilfe anzeigen:  rael --help
echo.
echo   HINWEIS: Öffne ein neues Terminal/PowerShell-Fenster,
echo            damit die PATH-Änderungen wirksam werden.
echo.

REM Test version
"%INSTALL_DIR%\rael.exe" --version 2>nul

pause
