@echo off
setlocal

cd /d "%~dp0.."

REM Build + staging live OUTSIDE Dropbox so Dropbox can't grab files mid-build.
set "BUILD_DIR=C:\Users\rich\sandbox\way\build"
set "STAGE_DIR=C:\Users\rich\sandbox\way"

set "LOG_FILE=%~dp0..\error.log"
echo === Build log %DATE% %TIME% === > "%LOG_FILE%"

REM Full path to powershell.exe — vcvarsall.bat reshuffles PATH and bare
REM `powershell` stops resolving after that. Cmake-wrapping calls rely on
REM this so they survive the env switch.
set "PS=C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe"

REM === Version prompt ===
REM Read current version by calling PowerShell DIRECTLY (not via for/f). for/f
REM runs the command through an extra cmd parse layer that mis-pairs the nested
REM quotes, so the read came back blank. Capture to a temp file and slurp the
REM first line with set /p — same single-parse path the write-back below uses.
set "CUR_VER="
"%PS%" -NoProfile -Command "$q=[char]34; if ((gc 'src/PluginProcessor.h' -Raw) -match ('PLUGIN_VERSION\s*=\s*'+$q+'([^'+$q+']+)'+$q)) { $Matches[1] }" > "%TEMP%\wayq_ver.txt"
set /p "CUR_VER=" < "%TEMP%\wayq_ver.txt"
del "%TEMP%\wayq_ver.txt" 2>nul
echo.
echo === Plugin Version ===
echo Current: %CUR_VER%
set "NEW_VER="
set /p "NEW_VER=Press Enter to keep, or type new version: "
if not "%NEW_VER%"=="" "%PS%" -NoProfile -Command "$q=[char]34; (gc 'src/PluginProcessor.h') -replace ('PLUGIN_VERSION\s*=\s*'+$q+'[^'+$q+']+'+$q), ('PLUGIN_VERSION = '+$q+'%NEW_VER%'+$q) | sc 'src/PluginProcessor.h'"
if not "%NEW_VER%"=="" (set "FINAL_VER=%NEW_VER%") else (set "FINAL_VER=%CUR_VER%")
echo.
set /p "BUILD_OK=Build %FINAL_VER%? [Y/n]: "
if /i "%BUILD_OK%"=="n" (echo Build cancelled. & pause & exit /b 0)
echo.

echo === WayQ Build ===
echo.
echo   Build dir: %BUILD_DIR%
echo.
echo   1. Quick           (incremental - only files that changed)
echo   2. Soft clean      (rebuild every .obj, keep JUCE cache + configure)
echo   3. Full            (wipe build dir, configure, build, install)
echo   0. Cancel
echo.
set /p "CHOICE=Choose [default=3]: "
if "%CHOICE%"=="" set "CHOICE=3"

if "%CHOICE%"=="1" goto build_quick
if "%CHOICE%"=="2" goto build_soft
if "%CHOICE%"=="3" goto build_full
echo Cancelled.
exit /b 0

REM ===========================================================================
:build_quick
echo.
echo --- Quick build ---
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 echo VCVARS FAILED
if errorlevel 1 set "BLDRC=1"
if errorlevel 1 goto :quick_done
"%PS%" -NoProfile -Command "cmake --build '%BUILD_DIR%' --config Release 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; exit $LASTEXITCODE"
set "BLDRC=%ERRORLEVEL%"
echo.
echo EXIT_CODE=%BLDRC%
if not "%BLDRC%"=="0" goto :quick_done
echo.
echo Install VST3 + CLAP to C:\Program Files\Common Files\{VST3,CLAP}\
set /p "CONFIRM=Install? (Y/n): "
if "%CONFIRM%"=="" set "CONFIRM=Y"
if /i "%CONFIRM%"=="Y" goto :quick_install_try
echo Skipped install.
goto :quick_done
:quick_install_try
xcopy /E /I /Y "%BUILD_DIR%\WayQ_artefacts\Release\VST3\WayQ.vst3" "C:\Program Files\Common Files\VST3\WayQ.vst3" >nul
if errorlevel 1 goto :quick_install_failed
if not exist "C:\Program Files\Common Files\CLAP" mkdir "C:\Program Files\Common Files\CLAP"
if exist "C:\Program Files\Common Files\CLAP" icacls "C:\Program Files\Common Files\CLAP" /grant *S-1-1-0:(OI)(CI)F >nul 2>&1
copy /Y "%BUILD_DIR%\WayQ_artefacts\Release\CLAP\WayQ.clap" "C:\Program Files\Common Files\CLAP\WayQ.clap" >nul
if errorlevel 1 goto :quick_install_failed
echo VST3 + CLAP installed.
echo [%DATE% %TIME%]
goto :quick_done
:quick_install_failed
echo.
echo VST3 or CLAP INSTALL FAILED. Possible causes: a DAW has the plugin locked, or this terminal lacks write access to Program Files (right-click the .bat and Run as administrator).
set /p "RETRY=[R]etry install or [Q]uit? "
if /i "%RETRY%"=="R" goto :quick_install_try
set "BLDRC=1"
:quick_done
echo.
echo Build log: %LOG_FILE%
set /p "DELLOG=Delete log? (Y/n): "
if "%DELLOG%"=="" set "DELLOG=Y"
if /i "%DELLOG%"=="Y" del "%LOG_FILE%" 2>nul
echo.
echo.
echo.
pause
exit /b %BLDRC%

REM ===========================================================================
:build_soft
REM Soft clean: rebuild every .obj but keep _deps/juce-src/ and CMakeCache.txt.
REM Sweet spot when you suspect a stale incremental but don't want a full nuke.
echo.
echo --- Soft-clean build ---
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 echo VCVARS FAILED
if errorlevel 1 set "BLDRC=1"
if errorlevel 1 goto :soft_done
"%PS%" -NoProfile -Command "cmake --build '%BUILD_DIR%' --config Release --clean-first 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; exit $LASTEXITCODE"
set "BLDRC=%ERRORLEVEL%"
echo.
echo EXIT_CODE=%BLDRC%
if not "%BLDRC%"=="0" goto :soft_done
echo.
echo Install VST3 + CLAP to C:\Program Files\Common Files\{VST3,CLAP}\
set /p "CONFIRM=Install? (Y/n): "
if "%CONFIRM%"=="" set "CONFIRM=Y"
if /i "%CONFIRM%"=="Y" goto :soft_install_try
echo Skipped install.
goto :soft_done
:soft_install_try
xcopy /E /I /Y "%BUILD_DIR%\WayQ_artefacts\Release\VST3\WayQ.vst3" "C:\Program Files\Common Files\VST3\WayQ.vst3" >nul
if errorlevel 1 goto :soft_install_failed
if not exist "C:\Program Files\Common Files\CLAP" mkdir "C:\Program Files\Common Files\CLAP"
if exist "C:\Program Files\Common Files\CLAP" icacls "C:\Program Files\Common Files\CLAP" /grant *S-1-1-0:(OI)(CI)F >nul 2>&1
copy /Y "%BUILD_DIR%\WayQ_artefacts\Release\CLAP\WayQ.clap" "C:\Program Files\Common Files\CLAP\WayQ.clap" >nul
if errorlevel 1 goto :soft_install_failed
echo VST3 + CLAP installed.
echo [%DATE% %TIME%]
goto :soft_done
:soft_install_failed
echo.
echo VST3 or CLAP INSTALL FAILED. Possible causes: a DAW has the plugin locked, or this terminal lacks write access to Program Files (right-click the .bat and Run as administrator).
set /p "RETRY=[R]etry install or [Q]uit? "
if /i "%RETRY%"=="R" goto :soft_install_try
set "BLDRC=1"
:soft_done
echo.
echo Build log: %LOG_FILE%
set /p "DELLOG=Delete log? (Y/n): "
if "%DELLOG%"=="" set "DELLOG=Y"
if /i "%DELLOG%"=="Y" del "%LOG_FILE%" 2>nul
echo.
echo.
echo.
pause
exit /b %BLDRC%

REM ===========================================================================
:build_full
echo.
echo --- Full build ---
set "BLDRC=0"

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

echo === WayQ Full Build ===
echo.
echo [1/3] Setting up MSVC environment...
call "C:\Program Files\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvarsall.bat" x64
if errorlevel 1 echo VCVARS FAILED
if errorlevel 1 goto :fail_done
echo.
echo [2/3] Configuring + building...
"%PS%" -NoProfile -Command "cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -S . -B '%BUILD_DIR%' 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; exit $LASTEXITCODE"
if errorlevel 1 echo CMAKE CONFIG FAILED
if errorlevel 1 goto :fail_done
"%PS%" -NoProfile -Command "cmake --build '%BUILD_DIR%' --config Release 2>&1 | Tee-Object -FilePath '%LOG_FILE%' -Append; exit $LASTEXITCODE"
if errorlevel 1 echo BUILD FAILED
if errorlevel 1 goto :fail_done
echo.
echo [3/3] Staging VST3 + CLAP...
:full_stage_try
if not exist "%STAGE_DIR%\" mkdir "%STAGE_DIR%"
xcopy /E /I /Y "%BUILD_DIR%\WayQ_artefacts\Release\VST3\WayQ.vst3" "%STAGE_DIR%\WayQ.vst3" >nul
if errorlevel 1 goto :full_stage_failed
copy /Y "%BUILD_DIR%\WayQ_artefacts\Release\CLAP\WayQ.clap" "%STAGE_DIR%\WayQ.clap" >nul
if errorlevel 1 goto :full_stage_failed
goto :full_stage_ok
:full_stage_failed
echo.
echo STAGING COPY FAILED. Possible causes: a DAW has the plugin locked, or the staging folder is in use.
set /p "RETRY=[R]etry staging or [Q]uit? "
if /i "%RETRY%"=="R" goto :full_stage_try
goto :fail_done
:full_stage_ok

echo.
echo Ready to install VST3 + CLAP to C:\Program Files\Common Files\{VST3,CLAP}\
set /p "CONFIRM=Overwrite plugin files? (Y/n): "
if "%CONFIRM%"=="" set "CONFIRM=Y"
if /i "%CONFIRM%"=="Y" goto :full_install_try
echo Skipped install.
goto :done
:full_install_try
xcopy /E /I /Y "%STAGE_DIR%\WayQ.vst3" "C:\Program Files\Common Files\VST3\WayQ.vst3" >nul
if errorlevel 1 goto :full_install_failed
if not exist "C:\Program Files\Common Files\CLAP" mkdir "C:\Program Files\Common Files\CLAP"
if exist "C:\Program Files\Common Files\CLAP" icacls "C:\Program Files\Common Files\CLAP" /grant *S-1-1-0:(OI)(CI)F >nul 2>&1
copy /Y "%STAGE_DIR%\WayQ.clap" "C:\Program Files\Common Files\CLAP\WayQ.clap" >nul
if errorlevel 1 goto :full_install_failed
echo VST3 + CLAP installed.
echo [%DATE% %TIME%]
goto :done
:full_install_failed
echo.
echo VST3 or CLAP INSTALL FAILED. Possible causes: a DAW has the plugin locked, or this terminal lacks write access to Program Files (right-click the .bat and Run as administrator).
set /p "RETRY=[R]etry install or [Q]uit? "
if /i "%RETRY%"=="R" goto :full_install_try
goto :fail_done

:fail_done
set "BLDRC=1"
goto :done

:done
echo.
echo Build log: %LOG_FILE%
set /p "DELLOG=Delete log? (Y/n): "
if "%DELLOG%"=="" set "DELLOG=Y"
if /i "%DELLOG%"=="Y" del "%LOG_FILE%" 2>nul
echo.
pause
exit /b %BLDRC%
