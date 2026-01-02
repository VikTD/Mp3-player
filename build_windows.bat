@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"

echo ==========================================
echo MP3 Player (Windows) - One-click build
echo Folder: %CD%
echo ==========================================
echo.

REM 0) Quick sanity checks
if not exist "scr\" (
  echo [ERROR] Missing folder: scr\
  echo Put this .bat in the project root (next to scr\, third_party\, assets\).
  pause
  exit /b 1
)

if not exist "third_party\" (
  echo [ERROR] Missing folder: third_party\
  pause
  exit /b 1
)

if not exist "assets\app.ico" (
  echo [ERROR] Missing icon: assets\app.ico
  pause
  exit /b 1
)

REM 1) Load Visual Studio build environment (MSVC + Windows SDK tools)
set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist %VSWHERE% (
  echo [ERROR] vswhere.exe not found.
  echo Install Visual Studio 2022 or Build Tools 2022 (Desktop development with C++).
  pause
  exit /b 1
)

for /f "usebackq delims=" %%I in (`%VSWHERE% -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSINSTALL=%%I

if not defined VSINSTALL (
  echo [ERROR] MSVC build tools not found.
  echo Fix: Visual Studio Installer -> Workloads -> Desktop development with C++
  pause
  exit /b 1
)

call "%VSINSTALL%\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul

where cl >nul 2>nul
if errorlevel 1 (
  echo [ERROR] cl.exe not found (MSVC not installed correctly).
  pause
  exit /b 1
)

REM 2) Find rc.exe (Windows SDK). Usually in PATH after VsDevCmd, but not always.
set RCEXE=rc
where rc >nul 2>nul
if errorlevel 1 (
  set RCEXE=
  for /f "delims=" %%D in ('dir /b /ad "C:\Program Files (x86)\Windows Kits\10\bin" 2^>nul ^| sort /r') do (
    if exist "C:\Program Files (x86)\Windows Kits\10\bin\%%D\x64\rc.exe" (
      set RCEXE="C:\Program Files (x86)\Windows Kits\10\bin\%%D\x64\rc.exe"
      goto :gotrc
    )
  )
  :gotrc
  if not defined RCEXE (
    echo [ERROR] rc.exe not found (Windows SDK missing).
    echo Fix: Visual Studio Installer -> Individual components -> Windows 10/11 SDK
    pause
    exit /b 1
  )
)

REM 3) Build folder + icon resource
if not exist "build\" mkdir build
echo 1 ICON "assets/app.ico" > build\app.rc
del /f /q build\app.res >nul 2>&1
%RCEXE% /nologo /fo build\app.res build\app.rc
if errorlevel 1 (
  echo [ERROR] Failed to compile icon (rc).
  echo Tip: make sure assets\app.ico is a real ICO (not a renamed PNG).
  pause
  exit /b 1
)

REM 4) Compile + link
del /f /q build\MusicPlayer.exe >nul 2>&1

cl /nologo /std:c17 /O2 /W3 /D_CRT_SECURE_NO_WARNINGS ^
  /I third_party /I scr ^
  scr\main.c scr\audio.c scr\playlist.c scr\config.c ^
  build\app.res ^
  /Fe"build\MusicPlayer.exe" ^
  /link ole32.lib uuid.lib shell32.lib winmm.lib

if errorlevel 1 (
  echo [ERROR] Build failed. Scroll up for the error.
  pause
  exit /b 1
)

echo.
echo ✅ Done! Output: build\MusicPlayer.exe
echo.
pause
