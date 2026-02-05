@echo off
REM ═══════════════════════════════════════════════════════════════════════════════
REM RAEL V48 SANG-REAL - All-In-One Windows 11 Build
REM ═══════════════════════════════════════════════════════════════════════════════
REM
REM Erstellt eine vollständige EXE mit allen Modulen aktiv:
REM   - RaelCore (Star8, Gate53, Ethics, Semantic, Resonance)
REM   - V49 Quint System (5D-Memory)
REM   - V49 AEYE Observer (Surveillance)
REM   - V49 JET Engine (61.440 Düsen)
REM   - V50 Alpha Shield (Aikido Defense)
REM   - V51 Security Daemon
REM   - V52 Frequency Interface
REM   - V53 Eternal Pulse
REM   - V56 World Simulation
REM   - Windows GUI (WinAPI)
REM   - WebGUI (HTTP Server auf Port 8080)
REM   - Voice TTS (SAPI)
REM
REM Voraussetzungen:
REM   - Windows 11
REM   - Visual Studio 2022 (Community Edition reicht)
REM   - Developer Command Prompt oder Visual Studio Terminal
REM
REM Ausführung:
REM   1. Öffne "Developer Command Prompt for VS 2022"
REM   2. cd zum rael_v48_clean/build Verzeichnis
REM   3. Build_AllInOne.bat
REM
REM ═══════════════════════════════════════════════════════════════════════════════

setlocal enabledelayedexpansion

echo.
echo ═══════════════════════════════════════════════════════════════════════════════
echo   R.A.E.L. V48 SANG-REAL - All-In-One Builder
echo   Identity: RAEL::ICH_BIN::IMMUTABLE::SIG_V1
echo   Michael-Signatur: 88.888...
echo ═══════════════════════════════════════════════════════════════════════════════
echo.

REM Pfade setzen
set ROOT=%~dp0..
set OUT=%ROOT%\bin
set MODOUT=%OUT%\modules
set INCLUDE=%ROOT%\include

REM Verzeichnisse erstellen
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%MODOUT%" mkdir "%MODOUT%"

REM Compiler-Flags für maximale Optimierung
set CL=/EHsc /std:c++17 /O2 /GL /I "%INCLUDE%" /D_WIN32 /DNOMINMAX /D_CRT_SECURE_NO_WARNINGS

REM Linker-Flags
set LINK=/LTCG

echo [1/6] Kompiliere RAEL Core-Bibliotheken...

REM Alle Core-Quelldateien
set CORE_SOURCES=^
 "%ROOT%\src\core\util.cpp" ^
 "%ROOT%\src\core\telemetry.cpp" ^
 "%ROOT%\src\core\metrics.cpp" ^
 "%ROOT%\src\core\events.cpp" ^
 "%ROOT%\src\core\improvements.cpp" ^
 "%ROOT%\src\core\lane_scheduler.cpp" ^
 "%ROOT%\src\core\star8.cpp" ^
 "%ROOT%\src\core\sha256.cpp" ^
 "%ROOT%\src\core\core_ring.cpp" ^
 "%ROOT%\src\core\pack.cpp" ^
 "%ROOT%\src\core\settings.cpp" ^
 "%ROOT%\src\core\ethics.cpp" ^
 "%ROOT%\src\core\ichbin.cpp" ^
 "%ROOT%\src\core\mathcore.cpp" ^
 "%ROOT%\src\core\semantic.cpp" ^
 "%ROOT%\src\core\nl_router.cpp" ^
 "%ROOT%\src\core\resonance.cpp" ^
 "%ROOT%\src\core\reflection_engine.cpp" ^
 "%ROOT%\src\core\attachments.cpp" ^
 "%ROOT%\src\core\module_manager.cpp" ^
 "%ROOT%\src\core\hotswap.cpp" ^
 "%ROOT%\src\core\voice.cpp" ^
 "%ROOT%\src\core\voicepack.cpp" ^
 "%ROOT%\src\core\parser.cpp" ^
 "%ROOT%\src\core\executor.cpp" ^
 "%ROOT%\src\core\codegen.cpp" ^
 "%ROOT%\src\core\debug_engine.cpp" ^
 "%ROOT%\src\core\lsp_server.cpp" ^
 "%ROOT%\src\core\raelcore.cpp"

echo [2/6] Kompiliere RAEL All-In-One CLI...

REM RAEL All-In-One mit allen Features
cl %CL% %CORE_SOURCES% "%ROOT%\src\cli\main.cpp" ^
   /Fe:"%OUT%\RAEL_V48_ALLINONE.exe" ^
   ole32.lib sapi.lib ws2_32.lib user32.lib gdi32.lib comctl32.lib shell32.lib advapi32.lib ^
   /link %LINK%
if errorlevel 1 (
    echo [ERROR] RAEL All-In-One build failed!
    goto :error
)
echo [OK] RAEL_V48_ALLINONE.exe erstellt

echo [3/6] Kompiliere RAEL GUI...

REM Windows GUI
if exist "%ROOT%\src\gui\rael_gui_win.cpp" (
    cl %CL% "%ROOT%\src\gui\rael_gui_win.cpp" ^
       /Fe:"%OUT%\RAEL_V48_GUI.exe" ^
       user32.lib gdi32.lib comctl32.lib ole32.lib sapi.lib ^
       /link %LINK%
    if errorlevel 1 (
        echo [WARN] GUI build failed, skipping...
    ) else (
        echo [OK] RAEL_V48_GUI.exe erstellt
    )
)

echo [4/6] Kompiliere RAEL WebGUI Server...

REM WebGUI mit HTTP Server
if exist "%ROOT%\src\webgui\rael_webgui_standalone.cpp" (
    cl %CL% "%ROOT%\src\webgui\rael_webgui_standalone.cpp" ^
       /Fe:"%OUT%\RAEL_V48_WEBGUI.exe" ^
       ws2_32.lib ^
       /link %LINK%
    if errorlevel 1 (
        echo [WARN] WebGUI build failed, skipping...
    ) else (
        echo [OK] RAEL_V48_WEBGUI.exe erstellt (http://localhost:8080)
    )
)

REM V56 Liquid-Blade
if exist "%ROOT%\src\webgui\rael_v56_liquid_blade.cpp" (
    cl %CL% "%ROOT%\src\webgui\rael_v56_liquid_blade.cpp" ^
       /Fe:"%OUT%\RAEL_V56_LIQUID.exe" ^
       ws2_32.lib ^
       /link %LINK%
    if errorlevel 1 (
        echo [WARN] V56 Liquid build failed, skipping...
    ) else (
        echo [OK] RAEL_V56_LIQUID.exe erstellt
    )
)

echo [5/6] Kompiliere Module (DLLs)...

REM Semantic Quint Module
if exist "%ROOT%\modules\sample_semantic_quint\sem_quint.cpp" (
    cl /LD %CL% "%ROOT%\modules\sample_semantic_quint\sem_quint.cpp" ^
       /Fe:"%MODOUT%\sem_quint.dll" ^
       /link %LINK%
    if errorlevel 1 (
        echo [WARN] sem_quint.dll build failed
    ) else (
        echo [OK] sem_quint.dll erstellt
    )
)

REM Math Formulas Module
if exist "%ROOT%\modules\sample_math_formulas\math_pack.cpp" (
    cl /LD %CL% "%ROOT%\modules\sample_math_formulas\math_pack.cpp" ^
       /Fe:"%MODOUT%\math_pack.dll" ^
       /link %LINK%
    if errorlevel 1 (
        echo [WARN] math_pack.dll build failed
    ) else (
        echo [OK] math_pack.dll erstellt
    )
)

echo [6/6] Kompiliere Tests...

REM Tests
for %%T in (01 02 03 04 05) do (
    if exist "%ROOT%\tests\test_%%T.cpp" (
        cl %CL% "%ROOT%\tests\test_%%T.cpp" /Fe:"%OUT%\test_%%T.exe" /link %LINK% 2>nul
        if not errorlevel 1 (
            echo [OK] test_%%T.exe
        )
    )
)

echo.
echo ═══════════════════════════════════════════════════════════════════════════════
echo   BUILD COMPLETE
echo ═══════════════════════════════════════════════════════════════════════════════
echo.
echo   Erstellte Dateien in: %OUT%
echo.
dir /B "%OUT%\*.exe" 2>nul
echo.
echo   Module in: %MODOUT%
echo.
dir /B "%MODOUT%\*.dll" 2>nul
echo.
echo ═══════════════════════════════════════════════════════════════════════════════
echo   STARTEN:
echo.
echo   CLI:      %OUT%\RAEL_V48_ALLINONE.exe
echo   GUI:      %OUT%\RAEL_V48_GUI.exe
echo   WebGUI:   %OUT%\RAEL_V48_WEBGUI.exe  (dann http://localhost:8080)
echo.
echo   Voice aktivieren:  "voice on" im CLI eingeben
echo   Module laden:      "load modules\sem_quint.dll"
echo ═══════════════════════════════════════════════════════════════════════════════
echo.
goto :end

:error
echo.
echo ═══════════════════════════════════════════════════════════════════════════════
echo   BUILD FAILED
echo ═══════════════════════════════════════════════════════════════════════════════
echo.
echo   Stellen Sie sicher, dass:
echo   1. Visual Studio 2022 installiert ist
echo   2. Sie die "Developer Command Prompt for VS 2022" verwenden
echo   3. Alle Quelldateien vorhanden sind
echo.
exit /b 1

:end
endlocal
