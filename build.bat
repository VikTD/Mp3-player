@echo off
setlocal

cd /d "%~dp0"

if not exist build mkdir build

REM Compile resources (icon etc.) if app.rc exists
if exist app.rc (
  echo [INFO] rc -> build\app.res
  rc /nologo /fo build\app.res app.rc
  if errorlevel 1 (
    echo [ERROR] rc failed.
    exit /b 1
  )
) else (
  echo [WARN] app.rc not found. Building without resources.
)

echo [INFO] cl -> build\Mp3Player.exe
cl /nologo /std:c17 /O2 /W3 /I third_party main.c build\app.res ^
  /Fe:build\Mp3Player.exe ^
  /link winmm.lib ole32.lib uuid.lib shell32.lib

if errorlevel 1 (
  echo [ERROR] build failed.
  exit /b 1
)

echo [OK] Done: build\Mp3Player.exe
endlocal
