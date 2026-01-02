@echo off
setlocal

cl /nologo /std:c17 /O2 /W3 /D_CRT_SECURE_NO_WARNINGS ^
  /I third_party /I scr ^
  scr\main.c scr\audio.c scr\playlist.c scr\config.c ^
  /FeMusicPlayer.exe ^
  /link ole32.lib uuid.lib shell32.lib winmm.lib

echo.
echo Build finished. Run: MusicPlayer.exe
endlocal