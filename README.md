# MP3 Player (Console) — C / Windows

Конзолен MP3 player, написан на C: зарежда MP3 файлове от папка, пуска/паузира, сменя песни, контролира звук и т.н.

> ✅ Platform: **Windows 10/11**  
> 🔊 Audio: **miniaudio** (single-header библиотека)

---

## ⚠️ WARNING

**ВНИМАНИЕ:** След като избереш папка с MP3 файлове, приложението **автоматично** пуска първата песен и то на **100% (максимален) звук**.

Преди да пуснете приложението:
- намали системния звук на Windows

Ако вече е пуснато и сте избрали папка с Mp3:
- натисни **6** и въведи стойност за звук **0–100** (например `10`), за да намалиш силата на звука.

---

## Готов .exe

Можеш да си изтеглиш готовата версия от **Releases** (в GitHub repo-то) и директно да стартираш `.exe` файла.

---

## Важно (прочети това)

За компилация **НЕ ползвай обикновен CMD или PowerShell**, защото там няма да са заредени нужните инструменти (`cl.exe`, `link.exe`, `rc.exe`) и библиотеките от Windows SDK (`winmm.lib` и др.).

✅ Компилирането става от:
**x64 Native Tools Command Prompt for VS** (или “Developer Command Prompt for VS”).

❌ Без: **CMD или PowerShell**

---

## Какво ти трябва (преди компилиране)

### 1) Visual Studio C++ Build Tools / Visual Studio Community
Трябва ти MSVC компилаторът (`cl.exe`) + Windows SDK.

Инсталирай едно от следните:
- **Visual Studio Build Tools 2022** (по-леко, само за компилиране), или
- **Visual Studio Community 2022** (IDE)

При инсталация отметни:
- ✅ **Desktop development with C++**
- ✅ **Windows 10/11 SDK**

---

## Използвани библиотеки (third-party)

- **miniaudio** (single-header) — `third_party/miniaudio.h`  
  Website: https://miniaud.io  
  GitHub: https://github.com/mackron/miniaudio

---

## Компилиране (автоматично) — build.bat

1) От Start меню отвори:
**x64 Native Tools Command Prompt for VS**(или “Developer Command Prompt for VS”)

2) Отиди в папката на проекта:
```bat
cd /d C:\path\to\repo
```

3) Стартирай билда:
```bat
build.bat
```

Резултат:
- `build\Mp3Player.exe`

---

## Компилиране (ръчно) — команди

> Пускай командите в **x64 Native Tools Command Prompt for VS**(или “Developer Command Prompt for VS”).

### Вариант A: С икона (app.rc → app.res)
```bat
cd /d C:\path\to\repo
mkdir build

rc /nologo /fo build\app.res app.rc

cl /nologo /std:c17 /O2 /W3 /I third_party main.c build\app.res ^
  /Fe:build\Mp3Player.exe ^
  /link winmm.lib ole32.lib uuid.lib shell32.lib
```

### Вариант B: Без икона (ако нямаш app.rc/app.ico)
```bat
cd /d C:\path\to\repo
mkdir build

cl /nologo /std:c17 /O2 /W3 /I third_party main.c ^
  /Fe:build\Mp3Player.exe ^
  /link winmm.lib ole32.lib uuid.lib shell32.lib
```

---

## Стартиране

```bat
build\Mp3Player.exe
```

---

## Чести проблеми

### `fatal error C1083: Cannot open include file: 'miniaudio.h'`
Провери да е наличен:
- `third_party\miniaudio.h`

и че компилираш с:
- `/I third_party`

### `cl is not recognized` / `rc is not recognized`
Не си отворил **x64 Native Tools Command Prompt for VS (или “Developer Command Prompt for VS”)** или нямаш инсталиран Windows SDK.
