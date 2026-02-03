@echo off
setlocal enabledelayedexpansion

set ROOT=%~dp0..
set OUT=%ROOT%\bin
set MODOUT=%OUT%\modules
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%MODOUT%" mkdir "%MODOUT%"

set CL=/EHsc /std:c++17 /O2 /I "%ROOT%\include"

set SOURCES=^
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
 "%ROOT%\src\core\raelcore.cpp" ^
 "%ROOT%\src\cli\main.cpp"

echo [build] core -> %OUT%\rael.exe
cl %CL% %SOURCES% /Fe:"%OUT%\rael.exe" ole32.lib sapi.lib
if errorlevel 1 exit /b 1

echo [build] modules -> %MODOUT%

REM semantic module dll
cl /LD %CL% "%ROOT%\modules\sample_semantic_quint\sem_quint.cpp" /Fe:"%MODOUT%\sem_quint.dll"
if errorlevel 1 exit /b 1

REM math module dll
cl /LD %CL% "%ROOT%\modules\sample_math_formulas\math_pack.cpp" /Fe:"%MODOUT%\math_pack.dll"
if errorlevel 1 exit /b 1

echo [build] done.

REM optional: Windows GUI cockpit (WinAPI)
if exist "%ROOT%\src\gui\rael_gui_win.cpp" (
  echo [build] gui  -> %OUT%\rael_gui.exe
  cl %CL% "%ROOT%\src\gui\rael_gui_win.cpp" /Fe:"%OUT%\rael_gui.exe" user32.lib gdi32.lib comctl32.lib
)

REM V56.2 Liquid-Blade WebGUI
if exist "%ROOT%\src\webgui\rael_v56_liquid_blade.cpp" (
  echo [build] V56.2 Liquid-Blade WebGUI -> %OUT%\rael_v56.exe
  cl %CL% "%ROOT%\src\webgui\rael_v56_liquid_blade.cpp" /Fe:"%OUT%\rael_v56.exe" ws2_32.lib
  if errorlevel 1 (
    echo [ERROR] V56.2 build failed
  ) else (
    echo [build] V56.2 Liquid-Blade ready: http://localhost:8080
  )
)

REM Legacy WebGUI
if exist "%ROOT%\src\webgui\rael_webgui_standalone.cpp" (
  echo [build] WebGUI  -> %OUT%\rael_webgui.exe
  cl %CL% "%ROOT%\src\webgui\rael_webgui_standalone.cpp" /Fe:"%OUT%\rael_webgui.exe" ws2_32.lib
)

endlocal
