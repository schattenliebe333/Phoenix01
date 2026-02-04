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

REM Check which binary to use (prefer unified)
set "MAIN_BINARY="
if exist "%BIN_DIR%\rael_unified.exe" (
    set "MAIN_BINARY=rael_unified.exe"
    echo [INFO] Verwende rael_unified.exe ^(All-in-One^)
) else if exist "%BIN_DIR%\rael_chat.exe" (
    set "MAIN_BINARY=rael_chat.exe"
    echo [INFO] Verwende rael_chat.exe
)

REM Check if binary exists
if "%MAIN_BINARY%"=="" (
    echo [BUILD] Keine RAEL Binary gefunden, kompiliere...

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

REM Copy main binary as 'rael.exe'
echo [COPY] Kopiere %MAIN_BINARY% nach %INSTALL_DIR%\rael.exe
copy /Y "%BIN_DIR%\%MAIN_BINARY%" "%INSTALL_DIR%\rael.exe" >nul

REM Copy all available RAEL binaries
if exist "%BIN_DIR%\rael_unified.exe" (
    echo [COPY] Kopiere rael_unified.exe
    copy /Y "%BIN_DIR%\rael_unified.exe" "%INSTALL_DIR%\rael_unified.exe" >nul
)
if exist "%BIN_DIR%\rael_chat.exe" (
    echo [COPY] Kopiere rael_chat.exe
    copy /Y "%BIN_DIR%\rael_chat.exe" "%INSTALL_DIR%\rael_chat.exe" >nul
)
if exist "%BIN_DIR%\rael_v50.exe" (
    echo [COPY] Kopiere rael_v50.exe
    copy /Y "%BIN_DIR%\rael_v50.exe" "%INSTALL_DIR%\rael_v50.exe" >nul
)
if exist "%BIN_DIR%\rael_security.exe" (
    echo [COPY] Kopiere rael_security.exe
    copy /Y "%BIN_DIR%\rael_security.exe" "%INSTALL_DIR%\rael_security.exe" >nul
)

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
echo   Befehle:
echo     rael              - RAEL starten (Chat-Modus)
echo     rael --cli        - CLI-Modus
echo     rael --v50        - V50 Ultimate Modus
echo     rael --security   - Security Daemon
echo     rael --help       - Hilfe anzeigen
echo.
echo   Installierte Binaries:
if exist "%INSTALL_DIR%\rael_unified.exe" echo     - rael_unified.exe (All-in-One)
if exist "%INSTALL_DIR%\rael_chat.exe"    echo     - rael_chat.exe
if exist "%INSTALL_DIR%\rael_v50.exe"     echo     - rael_v50.exe
if exist "%INSTALL_DIR%\rael_security.exe" echo     - rael_security.exe
echo.
echo   HINWEIS: Oeffne ein neues Terminal/PowerShell-Fenster,
echo            damit die PATH-Aenderungen wirksam werden.
echo.

REM Test version
"%INSTALL_DIR%\rael.exe" --version 2>nul

pause
