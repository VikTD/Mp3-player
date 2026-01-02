# 🎵 MP3 Player (C) — Windows (Console)

Конзолен MP3 плейър на **C** за Windows.

## Какво може (Features)
- Зареждане на папка с `.mp3` (**включително подпапки**)
- Playlist управление (Next/Prev + избор на текуща песен)
- Play / Pause / Resume
- Progress bar + време
- Volume, Auto-Next, Shuffle, Loop
- Смяна на папка през **File Explorer** (folder picker) + fallback към ръчно въвеждане

---

## 📁 Структура на проекта

- `scr/` — сорс кодът (C файлове)
  - `main.c` — меню/контроли, визуализация в терминала, input loop
  - `playlist.c/.h` — сканиране на папки (рекурсивно), филтриране `.mp3`, сортиране, next/prev
  - `audio.c/.h` — възпроизвеждане (miniaudio), pause/resume/seek, позиция/дължина
  - `config.c/.h` — запис/четене на настройки от `config.txt`
- `third_party/` — външни зависимости (single-header, включени в repo-то)
  - `miniaudio.h`
- `assets/`
  - `app.ico` — икона за `.exe` (вгражда се при билд)
- `config.txt` — настройки (volume/auto-next/shuffle/loop/sort)

---

## ⬇️ Изтегляне

### Вариант 1: ZIP (най-лесно)
GitHub → **Code → Download ZIP** → разархивираш.

### Вариант 2: Git clone
```bat
git clone https://github.com/VikTD/Mp3-player.git
cd Mp3-player
```

---

## ✅ Какво е нужно (Windows)

### Препоръчително
- **Visual Studio 2022** или **Build Tools 2022**
- Workload: **Desktop development with C++** (MSVC + Windows SDK)

> Не е нужно CMake. Проектът се компилира директно с `cl`.

---

## 🛠️ Компилиране (Windows):

### 1) One‑click (най-лесно)
В root папката има файл:
- `build_windows.bat`

Просто го стартираш (double‑click). Скриптът:
- намира Visual Studio tools (VsDevCmd)
- вгражда иконата (`assets/app.ico`) чрез `rc`
- компилира и прави: `build\MusicPlayer.exe`

✅ Резултатът е тук:
- `build\MusicPlayer.exe`

### 2) Ръчно (ако искаш да видиш командите)
Отвори **Developer Command Prompt for VS 2022**, после:

```bat
cd /d C:\path\to\Mp3-player

rc /nologo /fo build\app.res app.rc
cl /nologo /std:c17 /O2 /W3 /D_CRT_SECURE_NO_WARNINGS ^
  /I third_party /I scr ^
  scr\main.c scr\audio.c scr\playlist.c scr\config.c ^
  build\app.res ^
  /Fe"build\MusicPlayer.exe" ^
  /link ole32.lib uuid.lib shell32.lib winmm.lib
```

---

## ▶️ Стартиране
```bat
build\MusicPlayer.exe
```

---

## 📝 config.txt
`config.txt` съдържа **5 числа**, разделени със space:

```
<volume> <auto_next> <shuffle> <loop> <sort_mode>
```

Пример:
```
50 1 0 0 1
```

- `volume` : 0–100
- `auto_next` : 0/1
- `shuffle` : 0/1
- `loop` : 0/1
- `sort_mode` :
  - `0` = sort by full path
  - `1` = sort by file name
  - `2` = sort by folder, after that name

---
