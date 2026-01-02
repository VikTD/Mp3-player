@echo off
setlocal

REM Run this from: "Developer Command Prompt for VS 2022"
REM It builds MusicPlayer.exe and embeds the app icon.

if not exist assets\app.ico (
  echo ERROR: assets\app.ico not found
  exit /b 1
)

REM Create app.rc if missing
if not exist app.rc (
  echo 1 ICON "assets/app.ico" > app.rc
)

REM Compile resources (icon -> app.res)
where rc >nul 2>nul
if errorlevel 1 (
  echo ERROR: rc.exe not found. Install Windows SDK or use Developer Command Prompt.
  exit /b 1
)

rc /nologo /fo app.res app.rc || exit /b 1

REM Build exe
cl /nologo /std:c17 /O2 /W3 /D_CRT_SECURE_NO_WARNINGS ^
  /I third_party /I scr ^
  scr\main.c scr\audio.c scr\playlist.c scr\config.c ^
  app.res ^
  /FeMusicPlayer.exe ^
  /link ole32.lib uuid.lib shell32.lib winmm.lib || exit /b 1

echo.
echo OK: Built MusicPlayer.exe
endlocal
