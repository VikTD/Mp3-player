# 🎵 MP3 Player (C) — Windows (Console)

Конзолен MP3 плейър, написан на **C**, с поддръжка на:
- зареждане на папка с `.mp3` (включително **подпапки**)
- playlist (Next/Prev + избор на текуща песен)
- **Play / Pause / Resume**
- прогрес (таймер + progress bar)
- **Volume**, **Auto-Next**, **Shuffle**, **Loop**
- избор на папка през **File Explorer** (с fallback към ръчно въвеждане)

> Проектът е направен основно за **Windows** (ползва WinAPI/COM за folder picker и `_getch()` за input).

---

## Съдържание
- [Изтегляне](#изтегляне)
- [Какво е нужно](#какво-е-нужно)
- [Компилиране на Windows](#компилиране-на-windows)
  - [MSVC (препоръчително)](#msvc-препоръчително)
  - [MinGW-w64 (gcc)](#mingw-w64-gcc)
- [Стартиране](#стартиране)
- [Контроли](#контроли)
- [Конфигурация `config.txt`](#конфигурация-configtxt)
- [Структура на кода](#структура-на-кода)
- [Troubleshooting](#troubleshooting)
- [План / идеи](#план--идеи)

---

## Изтегляне

### Вариант 1: ZIP (най-лесно)
1) Натисни **Code → Download ZIP**  
2) Разархивирай проекта

### Вариант 2: Git clone
```bat
git clone https://github.com/VikTD/Mp3-player.git
cd Mp3-player
```

---

## Какво е нужно

### ✅ Препоръчително (MSVC)
- **Visual Studio 2022** или **Visual Studio 2022 Build Tools**
- Workload: **Desktop development with C++**  
  (да имаш `cl.exe` + Windows SDK)

### Алтернатива
- **MinGW-w64** (gcc) добавен в PATH

> **Забележка:** Проектът съдържа `third_party/miniaudio.h`, така че не се инсталират допълнителни аудио библиотеки.

---

## Компилиране на Windows

### MSVC (препоръчително)

1) От Start Menu отвори:
- **x64 Native Tools Command Prompt for VS 2022**  
  (или **Developer Command Prompt for VS 2022**)

2) Отиди в папката на проекта:
```bat
cd /d C:\path\to\Mp3-player
```

3) Компилирай:
```bat
cl /nologo /std:c17 /O2 /W3 /D_CRT_SECURE_NO_WARNINGS ^
  /I third_party /I scr ^
  scr\main.c scr\audio.c scr\playlist.c scr\config.c ^
  /FeMusicPlayer.exe ^
  /link ole32.lib uuid.lib shell32.lib winmm.lib
```

4) Стартирай:
```bat
MusicPlayer.exe
```

#### (По желание) Build скрипт
Можеш да добавиш файл `build_windows_msvc.bat` в root на repo-то:

```bat
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
```

---

### MinGW-w64 (gcc)

```bat
cd C:\path\to\Mp3-player

gcc -O2 -std=c17 -Ithird_party -Iscr ^
  scr\main.c scr\audio.c scr\playlist.c scr\config.c ^
  -o MusicPlayer.exe ^
  -lole32 -luuid -lshell32 -lwinmm
```

---

## Стартиране

- Увери се, че `config.txt` е в **същата папка**, от която стартираш програмата.
- При първо пускане/смяна на папка ще се отвори **folder picker**.
- Ако folder picker не се отвори, програмата позволява да въведеш пътя ръчно.

---

## Контроли

| Клавиш | Действие |
|---|---|
| `1` | Play / Resume |
| `2` | Pause |
| `3` | Next |
| `4` | Prev |
| `5` | Change folder (File Explorer) |
| `6` | Volume (0–100) |
| `7` | Auto-Next ON/OFF |
| `8` | Показва playlist |
| `9` | Shuffle ON/OFF |
| `L` | Loop ON/OFF |
| `A` | -5 секунди (seek назад) |
| `D` | +5 секунди (seek напред) |
| `S` | Sort: Path → Name → Folder |
| `0` | Exit |

---

## Конфигурация `config.txt`

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

Програмата обновява `config.txt` при промяна на настройки.

---

## Структура на кода

- `scr/main.c`  
  UI в терминала, input loop (`_getch()`), прогрес бар, избор на папка (WinAPI/COM), управление на playlist + аудио.
- `scr/playlist.c / playlist.h`  
  Сканира папка **рекурсивно**, филтрира `.mp3`, сортира (Path/Name/Folder), Next/Prev/Current.
- `scr/audio.c / audio.h`  
  Възпроизвеждане на MP3 чрез **miniaudio** (engine + sound), pause/resume/seek, позиция/продължителност.
- `scr/config.c / config.h`  
  Зареждане/запазване на настройките в `config.txt`.
- `third_party/miniaudio.h`  
  Single-header аудио библиотека.

---

## Troubleshooting

### `cl` не се разпознава
Отваряш грешен терминал. Ползвай **x64 Native Tools Command Prompt for VS 2022**.

### „No MP3 files found“
Провери дали:
- папката съдържа `.mp3` файлове
- имаш достъп до нея (permissions)

### Папки/файлове с кирилица
Folder picker връща Windows wide path, а проектът го конвертира към `char` със `wcstombs()`.  
Ако имаш проблеми с кирилица в пътищата:
- тествай с папка с латиница (пример: `C:\Music\Test`)
- или въведи пътя ръчно

---

## План / идеи
(от `to do list.txt`)
- да се запазва volume при следваща песен
- подобрен progress bar
- auto-play след смяна на песен
- по-добър loop/shuffle
